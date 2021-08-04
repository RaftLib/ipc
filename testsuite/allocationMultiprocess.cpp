#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <cstdint>
#include <thread>
#include <chrono>
#include <sys/wait.h>

#include "buffer.hpp"

void run_test( int proc_id )
{
    /** All procs must open the buffer **/
    auto *buffer = ipc::buffer::initialize( "thehandle" );

    /** Both test procs will attempt to allocate memory **/
    auto *fake_tls = ipc::buffer::get_tls_structure( buffer, proc_id );

    std::uint8_t *ptr = (std::uint8_t*) ipc::buffer::allocate_record( fake_tls, 128 );

    if( ptr == nullptr )
    {
        // FIXME - error, return something better than 0
        exit(0);
    }

    for( auto i( 0 ); i < 128; i++ )
    {
        //make sure memory is written to
        ptr[ i ] = i;
    }

    int offset = ipc::buffer::calculate_offset( buffer, (void*)ptr);

    /** We only have a few bit to return information in. Translate the offset to the block number **/
    offset /= (1 << ipc::buffer::block_size_power_two);

    /** Close tls and buffer **/
    ipc::buffer::close_tls_structure( fake_tls );
    ipc::buffer::destruct( buffer, "thehandle", false );

    /** Child proc exit **/
    exit(offset);
    return;

}

int main()
{

    /** Fork, then both processes will open up a buffer with the same handle, so
     * they should end up with the same shared memory, which can be used for
     * communication **/
    pid_t pid0, pid1;

    auto *buffer = ipc::buffer::initialize( "thehandle");

#ifdef DEBUG
    buffer->spin_lock = 1;
#endif

    // Fork first child
    switch( pid0 = fork() )
    {
        case 0: /* child */
            run_test(0);
            /* unreachable */
            break;
        case -1: /* parent, error forking */
            // FIXME - better error here
            exit(1);
            break;
        default: /* parent */
            break;
    }

    // Fork second child
    switch( pid1 = fork() )
    {
        case 0: /* child */
            run_test(1);
            /* unreachable */
            break;
        case -1: /* parent, error forking */
            // FIXME - better error here
            // Unlock other thread, wait for it to finish
#ifdef DEBUG
            buffer->spin_lock = 0;
#endif
            waitpid(pid0, NULL, 0);
            exit(1);
            break;
        default: /* parent */
            break;
    }

#ifdef DEBUG
    // Give the two spawned threads time to reach the spin lock in allocate, then unlock
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(0.25s);
    }

    buffer->spin_lock = 0;
#endif

    /** Wait on children to finish */
    int off0 = 0, off1 = 0;
    waitpid(pid0, &off0, 0);
    waitpid(pid1, &off1, 0);

    /** Close and unlink buffer **/
    ipc::buffer::destruct( buffer, "thehandle", true );

    /** Threads should have non-zero offset **/
    /** They returned the offsets of their allocations in their exit status **/

    if( off0 == 0 )
    {
        return( EXIT_FAILURE );
    }

    if( off1 == 0 )
    {
        return( EXIT_FAILURE );
    }

    /** Processes should recieve different data */
    if( off0 == off1 )
    {
        return( EXIT_FAILURE );
    }

    return( EXIT_SUCCESS );
}
