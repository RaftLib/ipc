#include <cstdlib>
#include <iostream>
#include <unistd.h>

#include <buffer>



int main()
{
    ipc::buffer::register_signal_handlers();
    shm_key_t key;
    ipc::buffer::gen_key( key, 42 );

    auto *buffer = ipc::buffer::initialize( key );
    ipc::buffer::destruct( buffer, key );
    return( EXIT_SUCCESS );
}
