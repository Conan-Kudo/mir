/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */
#ifndef MIR_CLIENT_MIR_CONNECTION_H_
#define MIR_CLIENT_MIR_CONNECTION_H_

#include "mir_wait_handle.h"
#include "lifecycle_control.h"
#include "ping_handler.h"
#include "rpc/mir_display_server.h"
#include "rpc/mir_display_server_debug.h"

#include "mir/geometry/size.h"
#include "mir/client_platform.h"
#include "mir/client_context.h"
#include "mir_toolkit/mir_client_library.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <unordered_map>

namespace mir
{
namespace protobuf
{
class BufferStream;
class Connection;
class ConnectParameters;
class PlatformOperationMessage;
class DisplayConfiguration;
}
/// The client-side library implementation namespace
namespace client
{
class ConnectionConfiguration;
class ClientPlatformFactory;
class ClientBufferStream;
class ClientBufferStreamFactory;
class ConnectionSurfaceMap;
class DisplayConfiguration;
class EventHandlerRegister;

namespace rpc
{
class DisplayServer;
class DisplayServerDebug;
class MirBasicRpcChannel;
}
}

namespace input
{
namespace receiver
{
class InputPlatform;
}
}

namespace logging
{
class Logger;
}

namespace dispatch
{
class ThreadedDispatcher;
}
}

struct MirConnection : mir::client::ClientContext
{
public:
    MirConnection(std::string const& error_message);

    MirConnection(mir::client::ConnectionConfiguration& conf);
    ~MirConnection() noexcept;

    MirConnection(MirConnection const &) = delete;
    MirConnection& operator=(MirConnection const &) = delete;

    MirWaitHandle* create_surface(
        MirSurfaceSpec const& spec,
        mir_surface_callback callback,
        void * context);
    MirWaitHandle* release_surface(
            MirSurface *surface,
            mir_surface_callback callback,
            void *context);

    MirPromptSession* create_prompt_session();

    char const * get_error_message();

    MirWaitHandle* connect(
        const char* app_name,
        mir_connected_callback callback,
        void * context);

    MirWaitHandle* disconnect();

    MirWaitHandle* platform_operation(
        MirPlatformMessage const* request,
        mir_platform_operation_callback callback, void* context);

    void register_lifecycle_event_callback(mir_lifecycle_event_callback callback, void* context);

    void register_ping_event_callback(mir_ping_event_callback callback, void* context);
    void pong(int32_t serial);

    void register_display_change_callback(mir_display_config_callback callback, void* context);

    void populate(MirPlatformPackage& platform_package);
    MirDisplayConfiguration* create_copy_of_display_config();
    void available_surface_formats(MirPixelFormat* formats,
                                   unsigned int formats_size, unsigned int& valid_formats);

    std::shared_ptr<mir::client::ClientPlatform> get_client_platform();

    std::shared_ptr<mir::client::ClientBufferStream> make_consumer_stream(
       mir::protobuf::BufferStream const& protobuf_bs, std::string const& surface_name, mir::geometry::Size);

    mir::client::ClientBufferStream* create_client_buffer_stream(
        int width, int height,
        MirPixelFormat format,
        MirBufferUsage buffer_usage,
        mir_buffer_stream_callback callback,
        void *context);
    MirWaitHandle* release_buffer_stream(
        mir::client::ClientBufferStream*,
        mir_buffer_stream_callback callback,
        void *context);

    void release_consumer_stream(mir::client::ClientBufferStream*);

    static bool is_valid(MirConnection *connection);

    EGLNativeDisplayType egl_native_display();
    MirPixelFormat       egl_pixel_format(EGLDisplay, EGLConfig) const;

    void on_surface_created(int id, MirSurface* surface);
    void on_stream_created(int id, mir::client::ClientBufferStream* stream);

    MirWaitHandle* configure_display(MirDisplayConfiguration* configuration);
    void done_display_configure();

    MirWaitHandle* set_base_display_configuration(MirDisplayConfiguration const* configuration);
    void done_set_base_display_configuration();

    std::shared_ptr<mir::client::rpc::MirBasicRpcChannel> rpc_channel() const
    {
        return channel;
    }

    mir::client::rpc::DisplayServer& display_server();
    std::shared_ptr<mir::logging::Logger> const& the_logger() const;

private:
    void populate_server_package(MirPlatformPackage& platform_package) override;
    // MUST be first data member so it is destroyed last.
    struct Deregisterer
    { MirConnection* const self; ~Deregisterer(); } deregisterer;

    mutable std::mutex mutex; // Protects all members of *this (except release_wait_handles)

    std::shared_ptr<mir::client::rpc::MirBasicRpcChannel> const channel;
    mir::client::rpc::DisplayServer server;
    mir::client::rpc::DisplayServerDebug debug;
    std::shared_ptr<mir::logging::Logger> const logger;
    std::unique_ptr<mir::protobuf::Void> void_response;
    std::unique_ptr<mir::protobuf::Connection> connect_result;
    std::atomic<bool> connect_done;
    std::unique_ptr<mir::protobuf::Void> ignored;
    std::unique_ptr<mir::protobuf::ConnectParameters> connect_parameters;
    std::unique_ptr<mir::protobuf::PlatformOperationMessage> platform_operation_reply;
    std::unique_ptr<mir::protobuf::DisplayConfiguration> display_configuration_response;
    std::unique_ptr<mir::protobuf::Void> set_base_display_configuration_response;
    std::atomic<bool> disconnecting{false};

    std::shared_ptr<mir::client::ClientPlatformFactory> const client_platform_factory;
    std::shared_ptr<mir::client::ClientPlatform> platform;
    std::shared_ptr<EGLNativeDisplayType> native_display;

    std::shared_ptr<mir::input::receiver::InputPlatform> const input_platform;

    std::string error_message;

    MirWaitHandle connect_wait_handle;
    MirWaitHandle disconnect_wait_handle;
    MirWaitHandle platform_operation_wait_handle;
    MirWaitHandle configure_display_wait_handle;
    MirWaitHandle set_base_display_configuration_wait_handle;

    std::mutex release_wait_handle_guard;
    std::vector<MirWaitHandle*> release_wait_handles;

    std::shared_ptr<mir::client::DisplayConfiguration> const display_configuration;

    std::shared_ptr<mir::client::LifecycleControl> const lifecycle_control;

    std::shared_ptr<mir::client::PingHandler> const ping_handler;

    std::shared_ptr<mir::client::ConnectionSurfaceMap> const surface_map;

    std::shared_ptr<mir::client::EventHandlerRegister> const event_handler_register;

    std::unique_ptr<google::protobuf::Closure> const pong_callback;

    std::unique_ptr<mir::dispatch::ThreadedDispatcher> const eventloop;
    
    std::shared_ptr<mir::client::ClientBufferStreamFactory> buffer_stream_factory;

    struct SurfaceRelease;
    struct StreamRelease;

    MirConnection* next_valid{nullptr};

    void set_error_message(std::string const& error);
    void done_disconnect();
    void connected(mir_connected_callback callback, void * context);
    void released(SurfaceRelease);
    void released(StreamRelease);
    void done_platform_operation(mir_platform_operation_callback, void* context);
    bool validate_user_display_config(MirDisplayConfiguration const* config);
};

#endif /* MIR_CLIENT_MIR_CONNECTION_H_ */
