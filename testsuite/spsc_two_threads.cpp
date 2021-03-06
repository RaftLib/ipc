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

#include <buffer>


using lf_queue_t = 
    ipc::spsc_lock_free_queue< ipc::channel_info      /** control region **/, 
                               void                   /** lf node        **/,
                               ipc::translate_helper  /** translator     **/>;

using gate_t = std::atomic< int >;

void producer(  const int count, 
                const ipc::channel_id_t channel_id, 
                ipc::buffer *buffer, 
                gate_t &g )
{
    const auto thread_id    = getpid();
    auto *tls_producer      = ipc::buffer::get_tls_structure( buffer, thread_id );
    if( ipc::buffer::add_spsc_lf_record_channel( tls_producer, channel_id, ipc::producer ) == ipc::channel_err )
    {
        return;
    }
    
    g++;
    while( g != 2 ){ __asm__ volatile ( "nop" : : : ); }

    for( int i( 0 ); i < count ; i++ )
    {
        int *output = 
            (int*) ipc::buffer::allocate_record( tls_producer, sizeof( int ), channel_id );
        *output = i;    
        while( ipc::buffer::send_record( tls_producer, channel_id, (void**)&output ) != ipc::tx_success );
    }
    ipc::buffer::unlink_channels( tls_producer );
    ipc::buffer::close_tls_structure( tls_producer );
    return;
}

void consumer(  const int count, 
                const ipc::channel_id_t channel_id, 
                ipc::buffer *buffer, 
                gate_t &g )
{
    const auto thread_id    = getpid();
    auto *tls_consumer      = ipc::buffer::get_tls_structure( buffer, thread_id );
    if( ipc::buffer::add_spsc_lf_record_channel( tls_consumer, channel_id, ipc::consumer ) == ipc::channel_err )
    {
        return;
    }
    
    g++;
    while( g != 2 ){ __asm__ volatile ( "nop" : : : ); }

    void *record = nullptr;
    auto count_tracker( 0 );
    int value = ~0;
    do
    {
        
        while( ipc::buffer::receive_record( tls_consumer, channel_id, &record ) != ipc::tx_success );
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
    //just remove all channels for this tls block, we only have one
    ipc::buffer::unlink_channels( tls_consumer );
    ipc::buffer::close_tls_structure( tls_consumer );
    return;
}

int main()
{
    ipc::buffer::register_signal_handlers();
    shm_key_t key;
    ipc::buffer::gen_key( key, 42 );
    auto *buffer = ipc::buffer::initialize( key  );

    auto channel_id = 1;

    gate_t gate = {0};
    


    //max count is 30 - 12 or (1<<18)
    //this should be bigger than the buffer size, we wanna make 
    //sure the allocator and everything else works. 
    const auto count = (1<<20);
    std::thread source( producer, count, channel_id, buffer, std::ref( gate ) );
    std::thread dest  ( consumer, count, channel_id, buffer, std::ref( gate ) );


    source.join();
    dest.join();
    
    ipc::buffer::destruct( buffer, key );

    return( EXIT_SUCCESS );
}
