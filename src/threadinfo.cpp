/**
 * threadinfo.cpp - 
 * @author: Jonathan Beard
 * @version: Tue Jul  6 14:46:46 2021
 * 
 * Copyright 2021 Jonathan Beard
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "threadinfo.hpp"
#include <cassert>

ipc::thread_info


ipc::thread_info::initialize( ipc::thread_info *th )
{
    /**
     * call constructor on itself given it hasn't 
     * been called at all b/c it was an inline 
     * in-memory allocation.
     */
    (th)->thread_info();
}
