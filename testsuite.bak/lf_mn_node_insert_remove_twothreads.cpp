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
#include "mpmc_lock_free.hpp"
#include "channelinfo.hpp"
#include "recordindex.hpp"
#include <functional>

#include <thread>
#include <atomic>

#undef DEBUG 

using lf_queue_t = 
    ipc::mpmc_lock_free_queue< ipc::channel_info      /** control region **/, 
                               ipc::record_index_t    /** lf node        **/,
                               ipc::translate_helper  /** translator     **/>;

using gate_t = std::atomic< int >;

void producer(  const std::size_t count, 
                ipc::channel_info *ch_info, 
                void *buffer, 
                gate_t &g )
{
    g++;
    while( g != 2 ){ __asm__ volatile ( "nop" : : : ); }

    char *tmp = (char*)buffer;
    for( std::size_t i( 0 ); i < count ; i++ )
    {
        const auto offset = 
            ((1<<ipc::block_size_power_two) * (i+1) /** zero is dummy**/ );
        assert( offset < (1<<30) );

        void *record_mem= (tmp + offset);
        auto *record = new (record_mem)
            ipc::record_index_t( i, 
                                 ipc::nodebase::normal, 
                                 offset /** similar to what the real one carries **/ );
        
        while( lf_queue_t::push( ch_info, record, buffer ) != ipc::tx_success );
    }
#if DEBUG    
    std::cout << "completed push sequence\n";
#endif    
    return;
}

void consumer( const std::size_t count, 
               ipc::channel_info *ch_info, 
               void *buffer, 
               gate_t &g )
{
    g++;
    while( g != 2 ){ __asm__ volatile ( "nop" : : : ); }

    std::size_t count_tracker( 0 );
    ipc::record_index_t *prev_record = nullptr;
    while( true )
    {
        ipc::record_index_t *record = nullptr;
        ipc::tx_code tx = ipc::tx_retry;
        while( tx != ipc::tx_success )
        {
            tx = lf_queue_t::pop( ch_info, &record, buffer );
        }
        if( record != nullptr )
        {
#if DEBUG
            std::cout << "(" << record->_id << ") - (" << count_tracker << ")\n";
            std::cout << *record << "\n";
            std::cout << *ch_info << "\n";
#endif
            if( record->_id != count_tracker)
            {
                std::cerr << "buffer failed async tests @ consumer, (" << 
                    record->_id << ") vs (" << count_tracker << ")\n";
                std::cerr << *prev_record << "\n";
                std::cerr << *record << "\n";
                exit( EXIT_FAILURE );
            }
            prev_record = record;
            count_tracker++;
            
            if( record->_id == (count - 1) )
            {
                break;
            }
            

        }
    }
#if DEBUG    
    std::cout << "count should be (131071)\n";
#endif    
    return;
}

int main()
{
    void *buffer    = malloc( (1 << 30) );
    auto *channel_info_obj = new ipc::channel_info( 0 );
    
    auto *dummy = new (buffer)   ipc::record_index_t
    (
        0 /** id **/,
        ipc::nodebase::dummy /** type **/

    );
    
    channel_info_obj->meta.dummy_node_offset = 4;
    gate_t gate = {0};

    //mpmc version of init
    lf_queue_t::init( channel_info_obj, dummy, buffer );

    //max count is 30 - 12 or (1<<18)
    const auto count = (1<<12);
    //const auto count = 16;
    std::thread source( producer, count, channel_info_obj, buffer, std::ref( gate ) );
    
    std::thread dest  ( consumer, count, channel_info_obj, buffer, std::ref( gate ) );


    source.join();
    dest.join();
    free( buffer );

    return( EXIT_SUCCESS );
}

#undef DEBUG
