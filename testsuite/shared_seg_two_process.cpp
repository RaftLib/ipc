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


struct shm_seg
{
    static void init( void *ptr )
    {
        auto *real_ptr = (shm_seg*)ptr;
        real_ptr->value = -1;
    }

    volatile std::int64_t value = -1;
};


/**
 * Here's what's going to happen, producer will send data to 
 * consumer, consumer will update shared segment, then the 
 * producer will read it and move on to the next one. If all
 * values are seen at the consumer then the test is passed,
 * otherwise it fails. 
 */

using gate_t = std::atomic< int >;

void producer(  const int count, 
                const ipc::channel_id_t channel_id, 
                ipc::buffer *buffer ) 
{
    const auto thread_id    = getpid();
    auto *tls_producer      = ipc::buffer::get_tls_structure( buffer, thread_id );
    std::cout << "beginning: " << 
        ipc::meta_info::heap_t::get_current_free( &tls_producer->buffer->heap  ) << "\n";
    if( ipc::buffer::add_spsc_lf_record_channel( tls_producer, 
                                                 channel_id, 
                                                 ipc::producer ) == ipc::channel_err )
    {
        std::cout << "channel error in producer\n";
        assert( false );
    }

    if( ipc::buffer::add_shared_segment( tls_producer,
                                         channel_id + 1 /** seg channel **/,
                                         sizeof( shm_seg ),
                                         shm_seg::init ) == ipc::channel_err )
    {
        std::cout << "channel error in producer\n";
        assert( false );
    }
    
    shm_seg *shared_seg = nullptr;
    if( ipc::buffer::open_shared_segment( tls_producer, 
                                          channel_id + 1, 
                                          (void**)&shared_seg ) != ipc::tx_success )
    {
        std::cout << "failed to open shared segment\n";
        assert( false );
    }
    for( int i( 0 ); i < count ; i++ )
    {
        int *output = 
            (int*) ipc::buffer::allocate_record( tls_producer, 
                                                 sizeof( int ), 
                                                 channel_id );
        *output = i;    
        //send
        while( ipc::buffer::send_record( tls_producer, 
                                         channel_id, 
                                         (void**)&output ) != ipc::tx_success );
        //spin till shared seg value is equal to what we expect
        while( shared_seg->value != i )
        {
            __asm__ volatile( "" : : "m" (shared_seg->value) );
        }
    }
    ipc::buffer::unlink_channels( tls_producer );
    std::cout << "end producer: " << 
        ipc::meta_info::heap_t::get_current_free( &tls_producer->buffer->heap  ) << "\n";
    ipc::buffer::close_tls_structure( tls_producer );
    return;
}

void consumer(  const int count, 
                const ipc::channel_id_t channel_id, 
                ipc::buffer *buffer ) 
{
    const auto thread_id    = getpid();
    auto *tls_consumer      = ipc::buffer::get_tls_structure( buffer, 
                                                              thread_id );

    if( ipc::buffer::add_spsc_lf_record_channel( tls_consumer, 
                                                 channel_id, 
                                                 ipc::consumer ) == ipc::channel_err )
    {
        std::cout << "channel error in consumer\n";
        return;
    }
    if( ipc::buffer::add_shared_segment( tls_consumer,
                                         channel_id + 1 /** seg channel **/,
                                         sizeof( shm_seg ),
                                         shm_seg::init ) == ipc::channel_err )
    {
        std::cout << "channel error in consumer\n";
        assert( false );
    }

    shm_seg *shared_seg = nullptr;
    if( ipc::buffer::open_shared_segment( tls_consumer, 
                                          channel_id + 1, 
                                          (void**)&shared_seg ) != ipc::tx_success )
    {
        std::cout << "failed to open shared segment in consumer\n";
        assert( false );
    }
    

    void *record = nullptr;
    auto count_tracker( 0 );
    int value = ~0;
    do
    {
        
        while( ipc::buffer::receive_record( tls_consumer, 
                                            channel_id, 
                                            &record ) != ipc::tx_success );
        value = *(int*)record;
        shared_seg->value = value; 
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
    std::cout << "end consumer: " << 
        ipc::meta_info::heap_t::get_current_free( &tls_consumer->buffer->heap  ) << "\n";
    ipc::buffer::close_tls_structure( tls_consumer );
    return;
}

int main( int argc, char **argv )
{
    ipc::buffer::register_signal_handlers();

    const auto channel_id   = 1;

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
    
    auto *buffer = ipc::buffer::initialize( "thehandle"  );
    if( is_producer )
    {
        producer(  count, 
                   channel_id, 
                   buffer );

        //we'll make the producer the main
        int status = 0;
        waitpid( -1, &status, 0 );
        ipc::buffer::destruct( buffer, "thehandle" );
    }
    else
    {
        consumer(  count, 
                   channel_id, 
                   buffer );
        ipc::buffer::destruct( buffer, "thehandle", false );

    }
    //buffer shouldn't destruct completely till everybody 
    //is done using it. 
    return( EXIT_SUCCESS );
}
