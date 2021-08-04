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
    auto *buffer = ipc::buffer::initialize( "thehandle");

    /** Both test procs will attempt to allocate memory **/
    auto *fake_tls = ipc::buffer::get_tls_structure( buffer, proc_id, 0 );

    /** Add ourselves to the index, then remove ourselsve.
     * Ignore the returns for this tests becaus we're only checking that
     * our counter works */

    /** Close tls and buffer **/
    ipc::buffer::close_tls_structure( fake_tls );
    ipc::buffer::destruct( buffer, "thehandle", false );

    /** Child proc exit **/
    exit(0);
    return;

}

int main()
{

    /** Fork n_threads threads, then check if the node count is accurate at the end */
    auto *buffer = ipc::buffer::initialize( "thehandle");

    auto const n_threads(10);
    std::array<pid_t, n_threads> pid;

    for( auto i(0); i < n_threads; i++ )
    {
        switch( pid[i] = fork() )
        {
            case 0: /* child */
                run_test(i);
                /* unreachable */
                break;
            case -1: /* parent, error forking */
                // FIXME - better error here
                exit(1);
                break;
            default: /* parent */
                break;
        }
    }

    /** Wait on children to finish */
    for( auto i(0); i < n_threads; i++ )
    {
        waitpid(pid[i], nullptr, 0);
    }

    auto node_count = ipc::buffer::channel_list_t::size( &buffer->channel_list );

    /** Close and unlink buffer **/
    ipc::buffer::destruct( buffer, "thehandle", true );

    if( node_count != n_threads )
    {
        return( EXIT_FAILURE );
    }

    return( EXIT_SUCCESS );
}
