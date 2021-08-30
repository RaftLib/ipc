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
   
    /** only do this for debugging **/
    std::cout << 
        ipc::meta_info::heap_t::get_current_free( &fake_tls->buffer->heap  ) << " - should be (262144)\n"; 
    //at this point we should have 262,144 blocks avail

    //we're allocating one channel with it's meta data so should be exactly
    //2 blocks 
    const auto ret_code  = ipc::buffer::add_spsc_lf_record_channel( fake_tls, channel_id );
    
    switch( ret_code )
    {
        case( ipc::channel_alloc_err ):
        {
            std::cerr << "alloatione error for channel\n";
        }
        break;
        default:
        {

        }
    }
    


    /** only do this for debugging **/
    std::cout << 
        ipc::meta_info::heap_t::get_current_free( &fake_tls->buffer->heap  ) << " - should be (262140)\n"; 
    
    switch( ret_code )
    {
        case( ipc::channel_alloc_err ):
        {
            std::cerr << "alloatione error for channel\n";
        }
        break;
        default:
        {

        }
    }
    
    //we're allocating the minimum buffer size which is 1MiB or 256 blocks (should have 
    std::uint8_t *ptr  = (std::uint8_t*) ipc::buffer::allocate_record( fake_tls, 128, channel_id );
    
    std::cout << 
        ipc::meta_info::heap_t::get_current_free( &fake_tls->buffer->heap  ) << " - should be (261884)\n"; 
    
    std::uint8_t *ptr2 = (std::uint8_t*) ipc::buffer::allocate_record( fake_tls, 128, channel_id + 1);
    
    //ptr2 should fail to allocate
    if( ptr2 == nullptr )
    {
        std::cout << "failed to return null when no channel exists\n";
        ipc::buffer::close_tls_structure( fake_tls );
        ipc::buffer::destruct( buffer, "thehandle" );
        //this is where we're supposed to fail 
        exit( EXIT_SUCCESS );
    }

    std::cout << 
        ipc::meta_info::heap_t::get_current_free( &fake_tls->buffer->heap  ) << " - should be (261628)\n"; 
    
    
    if( ptr == nullptr )
    {
        std::cerr << "Failed at allocate\n";
        ipc::buffer::close_tls_structure( fake_tls );
        ipc::buffer::destruct( buffer, "thehandle" );
        exit( EXIT_FAILURE );
    }
    for( auto i( 0 ); i < 128; i++ )
    {
        //make sure memory is written to
        ptr[ i ] = i; 
    }

    ipc::buffer::free_record( fake_tls, ptr2 );

    std::cout << 
        ipc::meta_info::heap_t::get_current_free( &fake_tls->buffer->heap  ) << " - should be (261630)\n"; 
    

    ipc::buffer::free_record( fake_tls, ptr );

    std::cout << 
        ipc::meta_info::heap_t::get_current_free( &fake_tls->buffer->heap  ) << " - should be (261632)\n"; 
    

    //blcoks that should be allocated are 258

    //buffer should have 261886 blocks remaining before closing tls structure
    
    ipc::buffer::close_tls_structure( fake_tls );
    
    std::cout << 
        ipc::meta_info::heap_t::get_current_free( &fake_tls->buffer->heap  ) << " - should be (262144)\n"; 

    ipc::buffer::destruct( buffer, "thehandle" );
    return( EXIT_SUCCESS );
}
