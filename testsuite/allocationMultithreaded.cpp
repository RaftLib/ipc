#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <cstdint>
#include <thread>
#include <chrono>
#include <future>

#include "buffer.hpp"

void run_test( ipc::buffer *buffer,
               std::promise<ipc::ptr_t> *offset,
               int thread_id)
{
    // Each thread gets its own tls
    auto *fake_tls = ipc::buffer::get_tls_structure( buffer, thread_id );

    std::uint8_t *ptr = (std::uint8_t*) ipc::buffer::allocate_record( fake_tls,128 );

    if( ptr == nullptr )
    {
        offset->set_value( 0 );
    }

    for( auto i( 0 ); i < 128; i++ )
    {
        //make sure memory is written to
        ptr[ i ] = i;
    }

    offset->set_value( ipc::buffer::calculate_offset( buffer, (void*)ptr) );

    ipc::buffer::close_tls_structure( fake_tls );

    return;

}

int main()
{

    /** One thread makes the buffer **/
    auto *buffer = ipc::buffer::initialize( "thehandle");

#ifdef DEBUG
    buffer->spin_lock = 1;
#endif

    std::promise<ipc::ptr_t> offset1;
    std::promise<ipc::ptr_t> offset2;

    auto off1_future = offset1.get_future();
    auto off2_future = offset2.get_future();

    std::thread t1(run_test, buffer, &offset1, 1);
    std::thread t2(run_test, buffer, &offset2, 2);


#ifdef DEBUG
    // Give the two spawned threads time to reach the spin lock in allocate
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(0.25s);
    }

    buffer->spin_lock = 0;
#endif

    t1.join();
    t2.join();

    /** Threads join, one thread closes the buffer **/
    ipc::buffer::destruct( buffer, "thehandle", true );

    /** Threads should have non-zero offset */
    auto off1 = off1_future.get();
    auto off2 = off2_future.get();

    if( off1 == 0 )
    {
        return( EXIT_FAILURE );
    }

    if( off2 == 0 )
    {
        return( EXIT_FAILURE );
    }

    /** Threads should recieve different data */
    if( off1 == off2 )
    {
        return( EXIT_FAILURE );
    }


    return( EXIT_SUCCESS );
}
