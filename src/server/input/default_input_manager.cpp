/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "default_input_manager.h"
#include "mir/input/platform.h"

#include "mir/main_loop.h"
#include "mir/thread_name.h"
#include "mir/terminate_with_current_exception.h"

#include <condition_variable>
#include <iostream>

namespace mi = mir::input;

mi::DefaultInputManager::DefaultInputManager(std::shared_ptr<Platform> const& input_platform,
                                             std::shared_ptr<InputDeviceRegistry> const& registry,
                                             std::shared_ptr<MainLoop> const& input_event_loop)
    : input_platform(input_platform), input_event_loop(input_event_loop), input_device_registry(registry), input_handler_register(input_event_loop), thread_state{State::stopped}
{
}

mi::DefaultInputManager::~DefaultInputManager()
{
    stop();
}

void mi::DefaultInputManager::start()
{
    State expected = State::stopped;
    while (!thread_state.compare_exchange_weak(expected, State::starting))
    {
        if (expected == State::running || expected == State::starting)
            return;
    }

    std::mutex m;
    std::condition_variable cv;
    bool startup_done = false;
    this->input_event_loop->enqueue(
        this,
        [&,this]()
        {
            input_platform->start(input_handler_register, input_device_registry);
            std::lock_guard<std::mutex> lk(m);
            {
                startup_done = true;
            }
            cv.notify_one();
        });
    input_thread = std::thread(
        [this]()
        {
            mir::set_thread_name("Mir/Input");
            try
            {
                this->input_event_loop->run();
            } catch(...)
            {
                mir::terminate_with_current_exception();
            }
        });

    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, [&startup_done]{return startup_done;});
    thread_state = State::running;
}

void mi::DefaultInputManager::stop()
{
    State expected = State::running;
    while (!thread_state.compare_exchange_weak(expected, State::stopping))
    {
        if (expected == State::stopped)
            return;
    }
    input_handler_register.register_handler(
        [this]()
        {
            input_platform->stop(input_handler_register);
        });

    input_event_loop->stop();
    if (input_thread.joinable())
    {
        input_thread.join();
    }
    thread_state = State::stopped;
}

