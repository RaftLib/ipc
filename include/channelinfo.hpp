/**
 * threadinfo.hpp - 
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
#ifndef _CHANNELINFO_HPP_
#define _CHANNELINFO_HPP_  1
#include <cstdint>
#include <unistd.h>
#include <cassert>
#include <iostream>
#include "bufferdefs.hpp"

/** ch_info components **/
#include "channelinfo_base.hpp"
#include "ch_ctrl_spsc.hpp"
#include "ch_entries_spsc.hpp"
#include "ch_data_spsc.hpp"


namespace ipc
{

/**
 * basic one which is used to cast
 * just about everywhere to get the 
 * meta-information for each channel 
 * type. 
 */
struct channel_info : public ipc::channel_info_base 
{
public:
    constexpr channel_info() = default;

    constexpr channel_info( const ipc::channel_id_t ch_id ) : 
        ipc::channel_info_base( ch_id ), 
        ctrl_spsc()
    {

    }


    ipc::ch_ctrl_spsc       ctrl_spsc;
};


/**
 * variation specifically for record types
 */
struct channel_info_record  : public ipc::channel_info
{
public:
    constexpr channel_info_record() = default;

    constexpr channel_info_record( const ipc::channel_id_t ch_id ) : 
        ipc::channel_info( ch_id ), 
        spsc_q() 
    {

    }


    ipc::ch_entries_spsc    spsc_q; 
};


/**
 * struct type with structures for typed in-band
 * data queue. 
 */
template < class T, int entries >
struct channel_info_inband_data  : public ipc::channel_info
{
public:
    constexpr channel_info_inband_data() = default;

    constexpr channel_info_inband_data( const ipc::channel_id_t ch_id ) : 
        ipc::channel_info( ch_id ), 
        spsc_q() 
    {

    }


    ipc::ch_data_spsc<T,entries>        spsc_q; 
};

inline 
std::ostream& operator << ( std::ostream &s, const channel_info &info )
{
    s << "{\n";
    s << (ipc::channel_info_base&)(info) << "\n";
    s << "\t" << info.ctrl_spsc.wrap_head << "\n";
    s << "\t" << info.ctrl_spsc.wrap_tail << "\n";
    s << "}";
    return( s );
}


} /** end namespace ipc **/

#endif /* END _CHANNELINFO_HPP_ */
