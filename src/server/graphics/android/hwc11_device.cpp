/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "hwc11_device.h"
#include "hwc_layerlist.h"
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

mga::HWC11Device::HWC11Device(std::shared_ptr<hwc_composer_device_1> const& hwc_device,
                              std::shared_ptr<HWCLayerOrganizer> const& organizer,
                              std::shared_ptr<DisplaySupportProvider> const& fbdev)
    : HWCCommonDevice(hwc_device),
      layer_organizer(organizer),
      fb_device(fbdev)
{
    size_t num_configs = 1;
    auto rc = hwc_device->getDisplayConfigs(hwc_device.get(), HWC_DISPLAY_PRIMARY, &primary_display_config, &num_configs);
    if (rc != 0)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("could not determine hwc display config")); 
    }
}

mga::HWC11Device::~HWC11Device() noexcept
{
}

geom::Size mga::HWC11Device::display_size() const
{
    static uint32_t size_request[3] = { HWC_DISPLAY_WIDTH,
                                        HWC_DISPLAY_HEIGHT,
                                        HWC_DISPLAY_NO_ATTRIBUTE};

    int size_values[2];
    /* note: some hwc modules (adreno320) do not accept any request list other than what surfaceflinger's requests,
     * despite what the hwc header says. from what I've seen so far, this is harmless, other than a logcat msg */ 
    hwc_device->getDisplayAttributes(hwc_device.get(), HWC_DISPLAY_PRIMARY, primary_display_config,
                                     size_request, size_values);

    return geom::Size{geom::Width{size_values[0]}, geom::Height{size_values[1]}};
}

geom::PixelFormat mga::HWC11Device::display_format() const
{
    return geom::PixelFormat::abgr_8888;
}

unsigned int mga::HWC11Device::number_of_framebuffers_available() const
{
    //note: the default for hwc devices is 2 framebuffers. However, the hwcomposer api allows for the module to give
    //us a hint to triple buffer. Taking this hint is currently not supported.
    return 2u;
}
 
void mga::HWC11Device::set_next_frontbuffer(std::shared_ptr<mga::AndroidBuffer> const& buffer)
{
    layer_organizer->set_fb_target(buffer);
    fb_device->set_next_frontbuffer(buffer);
}

void mga::HWC11Device::commit_frame(EGLDisplay dpy, EGLSurface sur)
{
    /* note, swapbuffers will go around through the driver and call set_next_frontbuffer */
    eglSwapBuffers(dpy, sur);

    auto& list = layer_organizer->native_list();

    auto struct_size = sizeof(hwc_display_contents_1_t) + sizeof(hwc_layer_1_t)*(list.size());
    auto hwc_display_raw = static_cast<hwc_display_contents_1_t*>( ::operator new( struct_size));
    auto hwc_display = std::unique_ptr<hwc_display_contents_1_t>(hwc_display_raw); 

    auto i = 0u;
    for( auto& layer : list)
    {
        hwc_display->hwLayers[i++] = *layer;
    }
    hwc_display->numHwLayers = list.size();
    hwc_display->retireFenceFd = -1;

    auto rc = hwc_device->set(hwc_device.get(), HWC_NUM_DISPLAY_TYPES, &hwc_display_raw);
    if (rc != 0)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("error during hwc set()"));
    }

    if (hwc_display->retireFenceFd > 0)
        close(hwc_display->retireFenceFd);
}
