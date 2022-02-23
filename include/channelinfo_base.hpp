/**
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
#ifndef _CHANNELINFO_BASE_HPP_
#define _CHANNELINFO_BASE_HPP_  1
#include <cstdint>
#include <unistd.h>
#include <cassert>
#include <iostream>
#include "bufferdefs.hpp"

/** ch_info components **/
#include "ch_meta_all.hpp"
#include "ch_ctrl_all.hpp"



namespace ipc
{

struct channel_info_base 
{
public:
    constexpr channel_info_base() = default;

    constexpr channel_info_base( const ipc::channel_id_t ch_id ) : meta( ch_id ),
                                                                   ctrl_all()
    {

    }


    ipc::ch_meta_all        meta;
    ipc::ch_ctrl_all        ctrl_all;
};

inline 
std::ostream& operator << ( std::ostream &s, const channel_info_base &info )
{
    s << "{\n";
    s << "  channel_id: " << info.meta.channel_id << ",\n";
    s << "  ref_count_prod:  " << info.meta.ref_count_prod << ",\n";
    s << "  ref_count_cons:  " << info.meta.ref_count_cons << ",\n";
    s << "  data_head: " <<  info.ctrl_all.data_head << ",\n";
    s << "  data_tail: " <<  info.ctrl_all.data_tail << "\n";
    s << "}";
    return( s );
}


} /** end namespace ipc **/

#endif /* END _CHANNELINFO_BASE_HPP_ */
