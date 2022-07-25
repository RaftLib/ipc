#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <cstdint>
#include <sys/types.h>
#include <unistd.h>

#include <buffer>



int main()
{
    
    ipc::buffer::register_signal_handlers(); 
    shm_key_t key;
    ipc::buffer::gen_key( key, 42 );
    auto *buffer = ipc::buffer::initialize( key  );

    auto channel_id = 1;
    auto thread_id = getpid();

    auto *fake_tls = ipc::buffer::get_tls_structure( buffer, thread_id );
    if( ipc::buffer::has_active_channels( fake_tls ) )
    {
        //should have no channels
        exit( EXIT_FAILURE );
    }
    ipc::buffer::add_spsc_lf_record_channel( fake_tls, channel_id, ipc::producer );
    
    if( ipc::buffer::has_channel( fake_tls, channel_id, false ) != true )
    {
        fprintf( stderr, "channel just added not found, exiting\n" );
        exit( EXIT_FAILURE );
    }

    ipc::buffer::has_channel( fake_tls, channel_id, true );

    fprintf( stderr, "channel found, destructing and exiting\n" );
    

    ipc::buffer::destruct( buffer, key );
    return( EXIT_SUCCESS );
}
