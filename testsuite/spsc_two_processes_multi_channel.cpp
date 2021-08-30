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
#include <functional>
#include <thread>
#include <atomic>
#include <sys/types.h>
#include <sys/wait.h>

#include <buffer>


using lf_queue_t = 
    ipc::spsc_lock_free_queue< ipc::channel_info      /** control region **/, 
                               void                   /** lf node        **/,
                               ipc::translate_helper  /** translator     **/>;

using gate_t = std::atomic< int >;

void producer(  const int count, 
                const ipc::channel_id_t channel_id_a, 
                const ipc::channel_id_t channel_id_b, 
                ipc::buffer *buffer ) 
{
    const auto thread_id    = getpid();
    auto *tls_producer      = ipc::buffer::get_tls_structure( buffer, thread_id );
    /** create two channels, single process, single thread **/
    if( ipc::buffer::add_spsc_lf_record_channel( tls_producer, 
                                          channel_id_a ) == ipc::channel_err )
    {
        return;
    }
    
    if( ipc::buffer::add_spsc_lf_record_channel( tls_producer, 
                                          channel_id_b ) == ipc::channel_err )
    {
        return;
    }
    
    for( int i( 0 ); i < count ; i++ )
    {
        //send channel 1
        {
            int *output = 
                (int*) ipc::buffer::allocate_record( tls_producer, 
                                                     sizeof( int ), 
                                                     channel_id_a );
            *output = i;    
            while( ipc::buffer::send_record(    tls_producer, 
                                                channel_id_a, 
                                                (void**)&output ) != ipc::tx_success );
        }
        //send channel 2
        {
            int *output = 
                (int*) ipc::buffer::allocate_record( tls_producer, 
                                                     sizeof( int ), 
                                                     channel_id_b );
            *output = i;    
            while( ipc::buffer::send_record(    tls_producer, 
                                                channel_id_b, 
                                                (void**)&output ) != ipc::tx_success );
        }
    }


    ipc::buffer::unlink_channels( tls_producer );
    ipc::buffer::close_tls_structure( tls_producer );
    std::cout << "completed push sequence\n";
    return;
}

void consumer(  const int count, 
                const ipc::channel_id_t channel_id, 
                ipc::buffer *buffer ) 
{
    const auto thread_id    = getpid();
    auto *tls_consumer      = ipc::buffer::get_tls_structure( buffer, thread_id );
    if( ipc::buffer::add_spsc_lf_record_channel( tls_consumer, channel_id ) == ipc::channel_err )
    {
        return;
    }
    

    void *record = nullptr;
    auto count_tracker( 0 );
    int value = ~0;
    do
    {
        
        while( ipc::buffer::receive_record(     tls_consumer, 
                                                channel_id, 
                                                &record ) != ipc::tx_success );
        value = *(int*)record;

        ipc::buffer::free_record( tls_consumer, record );
        
        if( value != count_tracker)
        {
            std::cerr << "buffer failed async tests @ consumer, (" << 
                value << ") vs (" << count_tracker << ")\n";
            exit( EXIT_FAILURE );
        }
        count_tracker++;
        
    }while( value != (count - 1) );
    ipc::buffer::remove_channel( tls_consumer, channel_id );
    ipc::buffer::close_tls_structure( tls_consumer );
    std::cout << "done with consumer channel (" << channel_id << ")\n";
    return;
}

int main()
{

    const auto channel_id_a   = 1;
    const auto channel_id_b   = 2;

    //max count is 30 - 12 or (1<<18)
    //this should be bigger than the buffer size, we wanna make 
    //sure the allocator and everything else works. 
    const auto count        = (1<<20);
    bool is_producer( false );
    auto child = fork();
    switch( child )
    {
        case( 0 /** child **/ ):
        {   
            //consumer
            is_producer = false;
        }
        break;
        case( -1 /** error, back to parent **/ ):
        {
            exit( EXIT_FAILURE );
        }
        break;
        default:
        {
            //producer
            is_producer = true;
        }
    }
    
    auto *buffer = ipc::buffer::initialize( "thehandle"  );
    if( is_producer )
    {
        producer(  count, 
                   channel_id_a,
                   channel_id_b,
                   buffer );
        //we'll make the producer the main
        int status = 0;
        waitpid( -1, &status, 0 );
        //buffer shouldn't destruct completely till everybody 
        //is done using it. 
        ipc::buffer::destruct( buffer, "thehandle" );
    }
    else
    {
        //thread one
        std::thread dest1( consumer, count, channel_id_a, buffer );
        //thread two
        std::thread dest2( consumer, count, channel_id_b, buffer );
        dest1.join();
        dest2.join();
        //unmap buffer from callee VA space
        ipc::buffer::destruct( buffer, "thehandle", false );
    }
    return( EXIT_SUCCESS );
}
