/**
 * genericnode.hpp - 
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

int main()
{
    void *buffer    = malloc( (1 << 20) );
    char *temp      = (char*)buffer;

   

    
    
    using lf_queue_t = 
        ipc::mpmc_lock_free_queue< ipc::channel_info      /** control region **/, 
                                   ipc::record_index_t    /** lf node        **/,
                                   ipc::translate_helper  /** translator     **/>;
    

    auto *channel_info_obj = 
        new (temp) ipc::channel_info( 0 );

    auto *dummy = 
        new (temp)   ipc::record_index_t;
    channel_info_obj->meta.dummy_node_offset = 4;


    lf_queue_t::init( channel_info_obj, dummy, buffer );

    const auto block_multiple = (1 << ipc::block_size_power_two);
    
    auto *record_0 = 
        new (temp + (1 << (ipc::block_size_power_two + 0 ) ))   ipc::record_index_t( 0, ipc::nodebase::normal, 10 );

    const auto record_0_offset = 
        ipc::translate_helper::calculate_block_offset( temp, record_0 );
    auto *record_1 = 
        new (temp + (block_multiple * 2 ))   
            ipc::record_index_t( 1, ipc::nodebase::normal, 11 );
    
    const auto record_1_offset = 
        ipc::translate_helper::calculate_block_offset( temp, record_1 );
    
    auto *record_2 = 
        new (temp + (block_multiple * 3))   
            ipc::record_index_t( 2, ipc::nodebase::normal, 12 );
    const auto record_2_offset = 
        ipc::translate_helper::calculate_block_offset( temp, record_2 );

    
    if( lf_queue_t::push( channel_info_obj, record_0, buffer ) != ipc::tx_success )
    {
        std::cerr << "failed to push record_0\n";
        free( buffer );
        exit( EXIT_FAILURE );
    }
    if( lf_queue_t::push( channel_info_obj, record_1, buffer ) != ipc::tx_success )
    {
        std::cerr << "failed to push record_1\n";
        free( buffer );
        exit( EXIT_FAILURE );
    }
    if( lf_queue_t::push( channel_info_obj, record_2, buffer ) != ipc::tx_success )
    {
        std::cerr << "failed to push record_2\n";
        free( buffer );
        exit( EXIT_FAILURE );
    }
   

    //check state of queue after three push ops
    if( dummy->next != record_0_offset )
    {
        std::cerr << "failed to set next offset correctly for record 0\n";
        exit( EXIT_FAILURE );
    }
    if( record_0->next != record_1_offset )
    {
        std::cerr << "failed to set next offset correctly for record 1\n";
        exit( EXIT_FAILURE );
    }
    if( record_1->next != record_2_offset )
    {
        std::cerr << "failed to set next offset correctly for record 2\n";
        exit( EXIT_FAILURE );
    }
    if( record_2->next != ipc::nodebase::init_offset() )
    {
        std::cerr << "failed to set next offset correctly for record 2\n";
        exit( EXIT_FAILURE );
    }

    
    ipc::record_index_t *record_ptr = nullptr;
    
    while( 
        lf_queue_t::pop(    channel_info_obj, 
                            &record_ptr, 
                            buffer ) != ipc::tx_success );
    
    if( record_ptr->_id != 0 )
    {
        std::cerr << *record_ptr << "\n";
        std::cerr << "node id should be (0)\n";
        exit( EXIT_FAILURE );
    }

    while( 
        lf_queue_t::pop(    channel_info_obj, 
                            &record_ptr, 
                            buffer ) != ipc::tx_success );
    
    if( record_ptr->_id != 1 )
    {
        std::cerr << *record_ptr << "\n";
        std::cerr << "node id should be (1)\n";
        exit( EXIT_FAILURE );
    }
    
    while( 
        lf_queue_t::pop(    channel_info_obj, 
                            &record_ptr, 
                            buffer ) != ipc::tx_success );
    
    if( record_ptr->_id != 2 )
    {
        std::cerr << *record_ptr << "\n";
        std::cerr << "node id should be (2)\n";
        exit( EXIT_FAILURE );
    }

    free( buffer );

    return( EXIT_SUCCESS );
}
