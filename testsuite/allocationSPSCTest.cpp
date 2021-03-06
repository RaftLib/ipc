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
   
    /** only do this for debugging **/
    std::cout << 
        ipc::meta_info::heap_t::get_current_free( &fake_tls->buffer->heap  ) << " - should be (262144)\n"; 
    //at this point we should have 262,144 blocks avail

    //we're allocating one channel with it's meta data so should be exactly
    //2 blocks 
    const auto ret_code = ipc::buffer::add_spsc_lf_record_channel( fake_tls, channel_id, ipc::producer );
    
    /** only do this for debugging **/
    std::cout << 
        ipc::meta_info::heap_t::get_current_free( &fake_tls->buffer->heap  ) << " - should be (262142)\n"; 
    
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
        ipc::meta_info::heap_t::get_current_free( &fake_tls->buffer->heap  ) << " - should be (261886)\n"; 
    
    std::uint8_t *ptr2 = (std::uint8_t*) ipc::buffer::allocate_record( fake_tls, 128, channel_id );
    
    std::cout << 
        ipc::meta_info::heap_t::get_current_free( &fake_tls->buffer->heap  ) << " - should be (261886)\n"; 
    
    
    if( ptr == nullptr )
    {
        std::cerr << "Failed at allocate\n";
        ipc::buffer::destruct( buffer, key );
        exit( EXIT_FAILURE );
    }
    for( auto i( 0 ); i < 128; i++ )
    {
        //make sure memory is written to
        ptr[ i ] = i; 
    }

    ipc::buffer::free_record( fake_tls, ptr2 );

    std::cout << 
        ipc::meta_info::heap_t::get_current_free( &fake_tls->buffer->heap  ) << " - should be (261888)\n"; 
    

    ipc::buffer::free_record( fake_tls, ptr );

    std::cout << 
        ipc::meta_info::heap_t::get_current_free( &fake_tls->buffer->heap  ) << " - should be (261890)\n"; 
    

    //blcoks that should be allocated are 258

    //buffer should have 261886 blocks remaining before closing tls structure
    
    ipc::buffer::close_tls_structure( fake_tls );
    
    std::cout << 
        ipc::meta_info::heap_t::get_current_free( &fake_tls->buffer->heap  ) << " - should be (262144)\n"; 

    ipc::buffer::destruct( buffer, key );
    return( EXIT_SUCCESS );
}
