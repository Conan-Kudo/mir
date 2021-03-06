/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_GRAPHICS_MESA_GBM_BUFFER_H_
#define MIR_GRAPHICS_MESA_GBM_BUFFER_H_

#include "mir/graphics/buffer_basic.h"
#include "mir/renderer/gl/texture_source.h"
#include "mir/renderer/gl/texture_target.h"

#include <gbm.h>

#include <memory>
#include <limits>

namespace mir
{
namespace graphics
{
namespace common
{
class BufferTextureBinder;
}

namespace mesa
{

class GBMBuffer: public BufferBasic, public NativeBufferBase,
                 public renderer::gl::TextureSource,
                 public renderer::gl::TextureTarget
{
public:
    GBMBuffer(std::shared_ptr<gbm_bo> const& handle,
              uint32_t bo_flags,
              std::unique_ptr<common::BufferTextureBinder> texture_binder);
    GBMBuffer(const GBMBuffer&) = delete;
    ~GBMBuffer();

    GBMBuffer& operator=(const GBMBuffer&) = delete;

    virtual geometry::Size size() const override;

    virtual geometry::Stride stride() const;

    virtual MirPixelFormat pixel_format() const override;

    virtual std::shared_ptr<graphics::NativeBuffer> native_buffer_handle() const override;

    virtual void gl_bind_to_texture() override;
    virtual void bind() override;
    virtual void secure_for_render() override;

    void bind_for_write() override;

    NativeBufferBase* native_buffer_base() override;

private:
    std::shared_ptr<gbm_bo> const gbm_handle;
    uint32_t bo_flags;
    std::unique_ptr<common::BufferTextureBinder> const texture_binder;
    int prime_fd;
};

}
}
}


#endif // MIR_GRAPHICS_MESA_GBM_BUFFER_H_
