#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <cstdint>
#include <sys/types.h>
#include <unistd.h>

#include <buffer>



int main()
{
    
    
    auto *buffer = ipc::buffer::initialize( "thehandle"  );

    auto channel_id = 1;
    auto thread_id = getpid();

    auto *fake_tls = ipc::buffer::get_tls_structure( buffer, thread_id );
   
    ipc::buffer::add_spsc_lf_record_channel( fake_tls, channel_id );
    ipc::buffer::add_spsc_lf_record_channel( fake_tls, channel_id + 1 );
        
    auto channel_list = ipc::buffer::get_channel_list( fake_tls );
    for( auto &pair : (*channel_list) )
    {
    std::cout << pair.first << " - " << ipc::channel_type_names[ pair.second ] << "\n";
    }
    ipc::buffer::close_tls_structure( fake_tls );
    ipc::buffer::destruct( buffer, "thehandle" );
    return( EXIT_SUCCESS );
}
