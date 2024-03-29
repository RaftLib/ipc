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
    
    if( ipc::buffer::has_active_channels( fake_tls ) != true )
    {
        //should have one channel
        exit( EXIT_FAILURE );
    }
    
    ipc::buffer::add_spsc_lf_record_channel( fake_tls, channel_id + 1, ipc::producer );
    /** force close all **/ 
    ipc::buffer::close_tls_structure( fake_tls );
    if( ipc::buffer::has_active_channels( fake_tls ) )
    {
        //should have no channels, close forces them all to shut
        exit( EXIT_FAILURE );
    }
    ipc::buffer::destruct( buffer, key );
    return( EXIT_SUCCESS );
}
