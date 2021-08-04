/**
 * ch_entries_spsc.hpp - 
 * @author: Jonathan Beard
 * @version: Mon Aug  2 13:18:51 2021
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
#ifndef CH_ENTRIES_SPSC_HPP
#define CH_ENTRIES_SPSC_HPP  1

#include "bufferdefs.hpp"

namespace ipc
{

struct alignas( (1<<ipc::block_size_power_two) ) ch_entries_spsc
{
    
    static constexpr auto n_entries = 
        ( 1 << ipc::block_size_power_two) / sizeof( ipc::ptr_offset_t );

    /**
     * now for entries for spsc queue, for now let's just add to all, 
     * even if it does waste an extra 4KiB of space. 
     */
    
    ipc::ptr_offset_t entry[ n_entries ] = { ipc::invalid_ptr_offset };

}; 

} /** end namespace ipc **/

#endif /* END CH_ENTRIES_SPSC_HPP */
