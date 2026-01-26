#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <cstdint>

#include <buffer>



int main()
{


    char *base = nullptr;
    base = reinterpret_cast<char*>( std::malloc( (1 << ipc::buffer_size_pow_two ) ) );
    if( base == nullptr )
    {
        std::cerr << "malloc failed\n";
        return( EXIT_FAILURE );
    }

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
    const auto offset1 = ipc::buffer::calculate_buffer_offset( base, (void*)(base));
    const auto offset3 = ipc::buffer::calculate_buffer_offset( base, (void*)(base - 1));
    const auto offset5 = ipc::buffer::calculate_buffer_offset( base, (void*)(0));
    const auto offset6 = ipc::buffer::calculate_buffer_offset( base,
        (void*)(base + ( 1<<ipc::buffer_size_pow_two )+1 ) );
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif


    /** Offsets should be zero */
    if( offset1 != 0 )
    {
        std::cerr << "fail 0\n";
        return( EXIT_FAILURE );
    }

    /** Offsets for addresses before buffer->data are invalid */
    if( offset3 != ipc::invalid_ptr_offset )
    {
        std::cerr << "fail 2\n";
        return( EXIT_FAILURE );
    }

    /** test zero given **/
    if( offset5 != ipc::invalid_ptr_offset  )
    {
        std::cerr << "fail 4\n";
        return( EXIT_FAILURE );
    }

    /** Test that address atk end of buffer is valid */
    if( offset6 != ipc::invalid_ptr_offset )
    {
        std::cerr << "fail 5\n";
        return( EXIT_FAILURE );
    }

    std::free( base );
    return( EXIT_SUCCESS );
}
