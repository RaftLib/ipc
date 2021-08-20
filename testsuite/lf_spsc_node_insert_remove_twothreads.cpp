/**
 * @author: Jonathan Beard
 * @version: Fri Sep 12 10:28:33 2014
 * 
 * Copyright 2014 Jonathan Beard
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
#include <cstdlib>
#include <typeinfo>
#include <iostream>
#include "genericnode.hpp"
#include "translate.hpp"
#include "spsc_lock_free.hpp"
#include "channelinfo.hpp"
#include "recordindex.hpp"
#include <functional>

#include <thread>
#include <atomic>



using lf_queue_t = 
    ipc::spsc_lock_free_queue< ipc::channel_info      /** control region **/, 
                               void                   /** lf node        **/,
                               ipc::translate_helper  /** translator     **/>;

using gate_t = std::atomic< int >;

void producer( const int count, ipc::channel_info *ch_info, void *buffer, gate_t &g )
{
    g++;
    while( g != 2 ){ __asm__ volatile ( "nop" : : : ); }

    char *tmp = (char*)buffer;
    for( int i( 0 ); i < count ; i++ )
    {
        const auto offset = ((1<<ipc::block_size_power_two) * i);
        assert( offset < (1<<30) );
        char *record = (tmp + offset);
        *((int*)record) = i;
        while( lf_queue_t::push( ch_info, record, buffer ) != ipc::tx_success );
    }
    return;
}

void consumer( const int count, ipc::channel_info *ch_info, void *buffer, gate_t &g )
{
    g++;
    while( g != 2 ){ __asm__ volatile ( "nop" : : : ); }

    int foo     = -1;
    int *record = &foo;
    auto count_tracker( 0 );
    do
    {
        ipc::tx_code tx = ipc::tx_retry;
        while( tx == ipc::tx_retry )
        {
            tx = lf_queue_t::pop( ch_info, (void**)&record, buffer );
        }
        if( *record != count_tracker)
        {
            std::cerr << "buffer failed async tests @ consumer, (" << 
                *record << ") vs (" << count_tracker << ")\n";
            exit( EXIT_FAILURE );
        }
        count_tracker++;
    }while( *record != (count - 1) );
    return;
}

int main()
{
    void *buffer    = malloc( (1 << 30) );
    auto *channel_info_obj = new ipc::channel_info( 0 );
    gate_t gate = {0};

    lf_queue_t::init( channel_info_obj );

    //max count is 30 - 12 or (1<<18)
    const auto count = (1<<17);
    std::thread source( producer, count, channel_info_obj, buffer, std::ref( gate ) );
    std::thread dest  ( consumer, count, channel_info_obj, buffer, std::ref( gate ) );


    source.join();
    dest.join();
    free( buffer );

    return( EXIT_SUCCESS );
}
