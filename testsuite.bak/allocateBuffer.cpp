#include <cstdlib>
#include <iostream>
#include <unistd.h>

#include "buffer.hpp"



int main()
{
    auto *buffer = ipc::buffer::initialize( "thehandle" );
    ipc::buffer::destruct( buffer, "thehandle", true );
    return( EXIT_SUCCESS );
}
