/**
 * ch_ctrl_all.hpp - 
 * @author: Jonathan Beard
 * @version: Mon Aug  2 13:14:19 2021
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
#ifndef CH_CTRL_ALL_HPP
#define CH_CTRL_ALL_HPP  1
#include "bufferdefs.hpp"
#include "padsize.hpp"

namespace ipc
{

struct alignas(L1D_CACHE_LINE_SIZE) ch_ctrl_all
{
    static constexpr auto ctrl_padding_constant = 
                    ipc::findpad< L1D_CACHE_LINE_SIZE, ipc::ptr_offset_t >::calc();
    
    /** start of data allocated **/
    alignas( L1D_CACHE_LINE_SIZE ) ipc::ctrl_ptroffset_t    data_head   = { ipc::invalid_ptr_offset };
    
    /** end of data allocated **/
    alignas( L1D_CACHE_LINE_SIZE ) ipc::ctrl_ptroffset_t    data_tail   = { ipc::invalid_ptr_offset };
};

} /** end namespace ipc **/
#endif /* END CH_CTRL_ALL_HPP */
