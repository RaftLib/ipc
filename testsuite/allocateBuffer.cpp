#include <cstdlib>
#include <iostream>
#include <unistd.h>

#include <buffer>



int main()
{
    ipc::buffer::register_signal_handlers();
    auto *buffer = ipc::buffer::initialize( "thehandle" );
    ipc::buffer::destruct( buffer, "thehandle" );
    return( EXIT_SUCCESS );
}
