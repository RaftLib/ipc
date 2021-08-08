/**
 * atomicregion.hpp - 
 * @author: Jonathan Beard
 * @version: Thu Apr  9 05:11:02 2020
 * 
 * Copyright 2020 Jonathan Beard
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
#ifndef _ATOMICREGION_HPP_
#define _ATOMICREGION_HPP_  1
#include <cstdint>
#include <unistd.h>
#include <cassert>
#include <iostream>
#include "bufferdefs.hpp"
#include "padsize.hpp"

/** ch_info components **/
#include "ch_meta_all.hpp"
#include "ch_ctrl_all.hpp"
#include "ch_ctrl_spsc.hpp"
#include "ch_entries_spsc.hpp"



namespace ipc
{
/**
 * FIXME - we need an assert to check cache line size
 * hasn't changed on the machine we're running on since
 * the algorithms rely on the cache line width. 
 */
struct atomic_region 
{
    using atomic_meta_t = ipc::ch_meta_all;

    constexpr atomic_region() = default;

    constexpr atomic_region( const ipc::channel_id_t ch_id ) : meta( ch_id ),
                                                              ctrl_all(),
                                                              ctrl_spsc(),
                                                              spsc_q() 
    {

    }


    atomic_meta_t        meta;
    
    static constexpr auto n_entries = 
        ( 1 << ipc::block_size_power_two) / sizeof( ipc::ptr_offset_t );

    /**
     * now for entries for spsc queue, for now let's just add to all, 
     * even if it does waste an extra 4KiB of space. 
     */
    
    ipc::ptr_offset_t entry[ n_entries ] = { ipc::invalid_ptr_offset };
};

inline 
std::ostream& operator << ( std::ostream &s, const atomic_region &info )
{
    s << "{\n";
    s << "  channel_id: " << info.meta.channel_id << ",\n";
    s << "  ref_count:  " << info.meta.ref_count << ",\n";
    s << "  data_head: " <<  info.ctrl_all.data_head << ",\n";
    s << "  data_tail: " <<  info.ctrl_all.data_tail << ",\n";
    s << "}";
    return( s );
}


} /** end namespace ipc **/

#endif /* END _ATOMICREGION_HPP_ */
