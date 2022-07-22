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



void producer(  const int count, 
                const ipc::channel_id_t channel_id, 
                ipc::buffer *buffer ) 
{
    const auto thread_id    = getpid();
    auto *tls_producer      = ipc::buffer::get_tls_structure( buffer, thread_id );
    if( ipc::buffer::add_spsc_lf_record_channel( tls_producer, channel_id, ipc::producer ) == ipc::channel_err )
    {
        return;
    }
    

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
                ipc::buffer *buffer ) 
{
    UNUSED( count );
    const auto thread_id    = getpid();
    auto *tls_consumer      = ipc::buffer::get_tls_structure( buffer, thread_id );
    if( ipc::buffer::add_spsc_lf_record_channel( tls_consumer, channel_id, ipc::consumer ) == ipc::channel_err )
    {
        return;
    }
    

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
        
    }while
    ( 
        ipc::buffer::channel_has_producers( tls_consumer, channel_id ) 
        ||
        (ipc::buffer::channel_has_data( tls_consumer, channel_id ) > 0)
    );
    //just remove all channels for this tls block, we only have one
    ipc::buffer::unlink_channels( tls_consumer );
    ipc::buffer::close_tls_structure( tls_consumer );
    return;
}

int main( int argc, char **argv )
{
    ipc::buffer::register_signal_handlers();

    const auto channel_id   = 1;
    shm_key_t key;
    ipc::buffer::gen_key( key, 42 );

    //max count is 30 - 12 or (1<<18)
    //this should be bigger than the buffer size, we wanna make 
    //sure the allocator and everything else works. 
    int count = (1 << 20 );
    if( argc > 1 )
    {
        count = (1 << atoi( argv[ 1 ] ) );
    }
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
    
    auto *buffer = ipc::buffer::initialize( key  );
    if( is_producer )
    {
        producer(  count, 
                   channel_id, 
                   buffer );

        //we'll make the producer the main
        int status = 0;
        waitpid( -1, &status, 0 );
        ipc::buffer::destruct( buffer, key );
    }
    else
    {
        consumer(  count, 
                   channel_id, 
                   buffer );
        ipc::buffer::destruct( buffer, key, false );

    }
    //buffer shouldn't destruct completely till everybody 
    //is done using it. 
    return( EXIT_SUCCESS );
}
