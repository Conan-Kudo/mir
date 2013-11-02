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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_DISPLAY_DEVICE_H_
#define MIR_TEST_DOUBLES_MOCK_DISPLAY_DEVICE_H_

#include "src/server/graphics/android/display_device.h"
#include <gmock/gmock.h>
 
namespace mir
{
namespace test
{
namespace doubles
{
class MockDisplayDevice : public graphics::android::DisplayDevice
{
public:
    ~MockDisplayDevice() noexcept {}
    MOCK_CONST_METHOD0(display_size, geometry::Size());
    MOCK_CONST_METHOD0(display_format, geometry::PixelFormat());
    MOCK_METHOD0(buffer_for_render, std::shared_ptr<graphics::Buffer>());
    MOCK_METHOD1(sync_to_display, void(bool));
    MOCK_METHOD1(mode, void(MirPowerMode));
    MOCK_METHOD2(commit_frame, void(EGLDisplay, EGLSurface));
};
}
}
}
#endif /* MIR_TEST_DOUBLES_MOCK_DISPLAY_DEVICE_H_ */
