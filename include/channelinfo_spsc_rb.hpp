/**
 * spsc_rb_channelinfo.hpp - basically a specialized template
 * class for the inline data ringbuffer.
 * 
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
#ifndef _CHANNELINFO_SPSC_RB_HPP_
#define _CHANNELINFO_SPSC_RB_HPP_  1
#include <cstdint>
#include <unistd.h>
#include <cassert>
#include <iostream>
#include "bufferdefs.hpp"
#include "padsize.hpp"

/** ch_info components **/
#include "channelinfo_base.hpp"
#include "ch_ctrl_spsc.hpp"
#include "ch_data_spsc.hpp"


namespace ipc
{

template < class T, int n_entries > 
    struct channel_info_spsc_rb : ipc::channel_info_base 
{
    constexpr channel_info_spsc_rb() = default;

    constexpr channel_info_spsc_rb( const ipc::channel_id_t ch_id ) : 
        ipc::channel_info_base( ch_id ),
        ctrl_spsc(),
        spsc_q() 
    {

    }


    ipc::ch_ctrl_spsc                   ctrl_spsc;
    //this is the onliy real difference between this class and the basic channel_info
    ipc::ch_data_spsc< T, n_entries >   spsc_q; 
};

inline 
std::ostream& operator << ( std::ostream &s, const channel_info_spsc_rb &info )
{
    s << "{\n";
    s << (ipc::channel_info_base&)(info) << "\n";
    s << "\t" << info.ctrl_spsc.wrap_head << "\n";
    s << "\t" << info.ctrl_spsc.wrap_tail << "\n";
    s << "\t" << typeid( T ).name() << "\n";
    s << "\tn_entries: " << n_entries << "\n";
    s << "}";
    return( s );
}


} /** end namespace ipc **/

#endif /* END _CHANNELINFO_SPSC_RB_HPP_ */
