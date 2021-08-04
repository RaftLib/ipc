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
#include "spsc_lock_free.hpp"
#include "channelinfo.hpp"
#include "recordindex.hpp"

int main()
{
    void *buffer    = malloc( (1 << 20) );
    char *temp      = (char*)buffer;

   

    
    
    using lf_queue_t = 
        ipc::spsc_lock_free_queue< ipc::channel_info      /** control region **/, 
                                   void                   /** lf node        **/,
                                   ipc::translate_helper  /** translator     **/>;
    

    auto *channel_info_obj = 
        new (temp) ipc::channel_info( 0 );



    lf_queue_t::init( channel_info_obj );

    const auto block_multiple = (1 << ipc::block_size_power_two);
    
    auto *record_0 = 
        (temp + (1 << (ipc::block_size_power_two + 0 ) ));   


    const auto record_0_offset = 
        ipc::translate_helper::calculate_block_offset( temp, record_0 );
    
    auto *record_1 = 
        (temp + (block_multiple * 2 )); 
    
    const auto record_1_offset = 
        ipc::translate_helper::calculate_block_offset( temp, record_1 );
    
    auto *record_2 = 
        (temp + (block_multiple * 3)); 

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
 
    if( lf_queue_t::size( channel_info_obj ) != 3 )
    {
        std::cerr << "failed to push, somwhere, queue size should be 3\n";
        free( buffer );
        exit( EXIT_FAILURE );
    }

    void *node_record_0 = nullptr;
    while( 
        lf_queue_t::pop( channel_info_obj, 
                         &node_record_0, 
                         buffer ) != ipc::tx_success );

    const auto node_record_0_offset = 
        ipc::translate_helper::calculate_block_offset( temp, node_record_0 );
    if( node_record_0_offset != record_0_offset )
    {
        std::cerr << "failed to get the offset we wanted\n";
        exit( EXIT_FAILURE );
    }
    //success for record 0
    
    if( lf_queue_t::size( channel_info_obj ) != 2 )
    {
        std::cerr << "failed to pop, somwhere, queue size should be 3\n";
        free( buffer );
        exit( EXIT_FAILURE );
    }
    
    void *node_record_1 = nullptr;
    while( 
        lf_queue_t::pop( channel_info_obj, &node_record_1, buffer ) != ipc::tx_success );
    
    const auto node_record_1_offset = 
        ipc::translate_helper::calculate_block_offset( temp, node_record_1 );
    if( node_record_1_offset != record_1_offset )
    {
        std::cerr << "failed to get the offset we wanted\n";
        exit( EXIT_FAILURE );
    }
    //success for record 1
    
    if( lf_queue_t::size( channel_info_obj ) != 1 )
    {
        std::cerr << "failed to pop, somwhere, queue size should be 3\n";
        free( buffer );
        exit( EXIT_FAILURE );
    }
    
    void *node_record_2 = nullptr;
    while( 
        lf_queue_t::pop( channel_info_obj, 
                         &node_record_2, 
                         buffer ) != ipc::tx_success );
    
    const auto node_record_2_offset = 
        ipc::translate_helper::calculate_block_offset( temp, node_record_2 );
    if( node_record_2_offset != record_2_offset )
    {
        std::cerr << "failed to get the offset we wanted\n";
        exit( EXIT_FAILURE );
    }
    //success for record 1
    
    if( lf_queue_t::size( channel_info_obj ) != 0 )
    {
        std::cerr << "failed to pop, somwhere, queue size should be 3\n";
        free( buffer );
        exit( EXIT_FAILURE );
    }


    free( buffer );

    return( EXIT_SUCCESS );
}
