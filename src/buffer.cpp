/**
 * buffer.cpp -
 * @author: Jonathan Beard
 * @version: Fri Mar 13 09:35:16 2020
 *
 * Copyright 2020 Jonathan Beard
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <fcntl.h>
#include <semaphore.h>
#include <cstring>
#include <unistd.h>
#include <shm>
#include <sstream>
#include <utility>
#include <type_traits>
#include <typeinfo>
#include <signal.h>

#include "sem.hpp"
#include <buffer>

#include <errno.h>
#include "allocation_metadata.hpp"
#include "allocationexception.hpp"

ipc::global_err_t ipc::buffer::gb_err;

void
ipc::buffer::shutdown_handler( int signum )
{
    std::cerr << "Shutting down ipc buffer\n";
    std::perror( ipc::buffer::gb_err.err_msg.str().c_str() );
    UNUSED( signum );
        
    //get rid of semaphores, doesn't matter if these fail
    ipc::sem::final_close( ipc::buffer::gb_err.buffer->index_sem_name );
    ipc::sem::final_close( ipc::buffer::gb_err.buffer->alloc_sem_name );
    
    std::cerr << "ipc::buffer::gb_err: " << ipc::buffer::gb_err.shm_handle << "\n";
    //give this a shot even if it's null
    shm::close( ipc::buffer::gb_err.shm_handle,
                (void**)&ipc::buffer::gb_err.buffer,
                ipc::buffer::gb_err.buffer->allocated_size,
                false,
                true );
}

void ipc::buffer::register_signal_handlers()
{
    struct sigaction action;
    sigemptyset( &action.sa_mask );
    action.sa_flags = 0;
    action.sa_handler = ipc::buffer::shutdown_handler;
    sigaction (SIGUSR1, &action, nullptr );
    sigaction (SIGSEGV, &action, nullptr );
    sigaction (SIGKILL, &action, nullptr );
    sigaction (SIGINT,  &action, nullptr );
}


ipc::buffer::buffer() : buffer_base()
{
    //nothing to see here
}

ipc::buffer*
ipc::buffer::initialize( const std::string shm_handle )
{
    //stupid sanity check
    assert( shm_handle.length() > 0 );

    ipc::buffer::gb_err.shm_handle = shm_handle;

    /** constants **/
    const auto buffer_size_nbytes   = 1 << ipc::buffer_size_pow_two;
    const auto meta_size_bytes      = sizeof( ipc::buffer_base );
    /** 
     * because of the way the structures are laid out, this should 
     * be a multiple of the overall page size.
     */
    const auto size_we_need         = buffer_size_nbytes + meta_size_bytes;

    void *memory( nullptr );
    memory = shm::init( shm_handle, 
                            size_we_need, 
                            false /** don't zero **/, 
                            nullptr /** no placement **/ );
    if( memory == (void*)-1)
    {
        auto *output( shm::eopen< ipc::buffer >( shm_handle ) );
        while( output->cookie.load( std::memory_order_seq_cst)
            != ipc::buffer::cookie_in_use )
        {
            //spin while parent is setting things up.
            __asm__ volatile( "nop" : : : );
        }
        ipc::buffer::gb_err.buffer = output;
        return( output );
    }
    
    //we're here, then we're the first ones, lots to do.
    /**
     * STEP 1, set up semaphore name and semaphore for alloc
     */
    /**
     * avoid duplicate code with lambda 
     */
    auto sem_allocate_f = []( 
        const std::string &&name, 
        const int proj_id,
        ipc::sem::sem_key_t &sem_name
        )->auto
    {
        ipc::sem::generate_key( ipc::sem::semaphore_length - 1, 
                                proj_id,
                                sem_name );

        auto sem_id   = ipc::sem::open( sem_name,
                                        ipc::sem::sem_create,
                                        ipc::sem::file_rdwr );
        if( sem_id == ipc::sem::sem_error )
        {
            ipc::buffer::gb_err.err_msg << "Failed to open semaphore, exiting!!" ;
            shutdown_handler( 0 );
        }

        if( ipc::sem::main_init( sem_id ) == -1 )
        {
            ipc::buffer::gb_err.err_msg << "failed to init index semaphore, (" << name << "), exiting";
            shutdown_handler( 0 );
        }
        /**
         * changed from std::pair to returning the active/open
         * sem so we don't run into the ret val optimization bug
         */
        return( sem_id );    
    };
    ipc::sem::sem_key_t alloc_sem_name  = {0};
    auto sem_alloc = sem_allocate_f( "alloc", 0x13, alloc_sem_name );
    ipc::sem::sem_key_t index_sem_name  = {0};
    auto sem_index = sem_allocate_f( "index", 0x14, index_sem_name );
    

    /**
     * STEP 2, set up object allocation
     */
    assert( memory != nullptr );
    auto *out_buffer = new (memory) ipc::buffer();
    
    
    ipc::buffer::gb_err.buffer = out_buffer;
    /**
     * STEP 3, initialize data structures inside object
     * remember to copy over semaphore names
     * - index_start - offset should equal 0, which is invalid
     * - index_end   - offset should equal 0, which is invalid
     * - index_sem_name - generate semaphore name, copy here
     * - alloc_sem_name - generate alloc anme, copy here
     * - allocated_size - should be set to initial buffer size
     * - free_start_offset - set initially to "data" start
     * - cookie, set to cookie value once all written
     */

    /** 
     * copy these after we have the semaphore, and 
     * after the constructor was called.
     */
    //strcopy index
    ipc::sem::key_copy( out_buffer->index_sem_name,
                        ipc::sem::semaphore_length - 1,
                        index_sem_name );


    //strcopy alloc sem name
    ipc::sem::key_copy( out_buffer->alloc_sem_name,
                        ipc::sem::semaphore_length - 1,
                        alloc_sem_name );
    

    out_buffer->allocated_size      = size_we_need;
    out_buffer->databuffer_size     = buffer_size_nbytes;
    
    ipc::sem::close( sem_alloc );
    ipc::sem::close( sem_index );

    out_buffer->cookie.store( ipc::buffer_base::cookie_in_use, 
                              std::memory_order_seq_cst);
    return( out_buffer );
}

std::string 
ipc::buffer::get_tmp_dir()
{
#if defined __linux 
    char *dir = getenv( "TMPDIR" );
    if( dir != nullptr )
    {
        return( std::string( dir ) );
    }
    else
    {
        return( "/tmp" );
    }
#else
#pragma message ("tmp directory helper not yet implemented for non-linux platforms, returning (./)")
    return( "./" );
#endif
}

void
ipc::buffer::destruct( ipc::buffer *b,
                       const std::string &shm_handle,
                       const bool unlink,
                       const bool unmap )
{
    if( unlink )
    {
        if( ipc::sem::final_close( b->index_sem_name ) == -1 )
        {
            std::stringstream errstr;
            errstr << "error on destruction with sem name (" << b->index_sem_name << ")";
            std::perror( errstr.str().c_str() );
        }
        if( ipc::sem::final_close( b->alloc_sem_name ) == -1 )
        {
            std::stringstream errstr;
            errstr << "error on destruction with sem name (" << b->alloc_sem_name << ")";
            std::perror( errstr.str().c_str() );
        }
    }
    if( unmap )
    {
        shm::close( shm_handle,
                    (void**)&b,
                    b->allocated_size,
                    false,
                    unlink );
    }
    return;
}

ipc::channel_id_t
ipc::buffer::add_channel( ipc::thread_local_data    *data,
                          const channel_id_t        channel_id, 
                          const ipc::channel_type   type,
                          const ipc::direction_t    dir )
{
    assert( data != nullptr );
    const auto size_to_allocate( sizeof( ipc::channel_index_t ) );
    
    const auto channel_info_multiple  = 
        ipc::buffer::heap_t::get_block_multiple( size_to_allocate );
        
    
    //we're here, we should have enough local mem
    const auto meta_multiple = 
        ipc::buffer::heap_t::get_block_multiple( sizeof( ipc::allocate_metadata ) );
   


    ipc::channel_info *channel = nullptr;

    ipc::channel_id_t channel_start = ipc::null_channel;

    /**
     * Acquire semaphore, must go after allocate otherwise we have 
     * nested semaphore acquire and deadlock.
     */
     //FIXME
    auto sem = data->index_semaphore;
    if( ipc::sem::wait( sem ) == ipc::sem::uni_error )
    {
        ipc::buffer::gb_err.err_msg << 
            "Failed to wait, plz debug at line (" << __LINE__ << ")" << 
             " with sem value: " << sem;
        shutdown_handler( 0 );
    }
    //returns block that is modulo block_size
    channel_start  = 
        ipc::buffer::find_channel_buffer_offset( data, channel_id, &channel );
    
    if( channel_start < ipc::valid_offset )
    {
        // Create new node -- will have to acquire allocation semaphore
        // BEWARE - we already grabbed index semaphore, they're now nested
        std::size_t blocks_to_allocate = channel_info_multiple + meta_multiple;
        
        void *mem_for_new_channel = 
            ipc::buffer::global_buffer_allocate(  data, 
                                                  blocks_to_allocate,
                                                  true ); 

        if( mem_for_new_channel == nullptr )
        {
            channel_start = ipc::channel_alloc_err;
            goto POST;

        }
        
        const auto meta_offset = 
            ipc::buffer::calculate_block_offset( &data->buffer->data, 
                                                 mem_for_new_channel );


        /** need to calculate channel start, don't change pointer yet, we need that **/
        channel_start = meta_offset + meta_multiple;

        //make an actual meta-data structure
        new (mem_for_new_channel) ipc::allocate_metadata( 
            channel_start           /** help the allocator find the base and de-allocate this block **/,
            channel_info_multiple   /** count of blocks allocated, not including metadata **/
        );
        
        
        mem_for_new_channel = ipc::buffer::translate_block( &data->buffer->data, channel_start );
    

        //initialize new channel structure
        auto *node_to_add = 
        new (mem_for_new_channel) ipc::channel_index_t( channel_id            /** id **/,
                                                        ipc::nodebase::normal /** node type **/,
                                                        channel_id );
        channel = &(**node_to_add);
    
        switch( channel->meta.type )
        {
            case( ipc::mpmc_record ):
            {
                //set up dummy node for LF queue 
                auto dummy_info_multiple  = 
                    ipc::buffer::heap_t::get_block_multiple( sizeof( ipc::record_index_t ) );
                
                void *mem_for_dummy_node = 
                    ipc::buffer::global_buffer_allocate(  data, 
                                                          dummy_info_multiple,
                                                          true ); 


                auto *dummy_record = new (mem_for_dummy_node) ipc::record_index_t( ipc::nodebase::dummy );
                

                channel->meta.dummy_node_offset = 
                    ipc::buffer::calculate_block_offset( &data->buffer->data, dummy_record );
                ipc::buffer::mpmc_lock_free::init( channel, 
                                                   dummy_record,
                                                   &data->buffer->data );
            }
            break;
            case( ipc::spsc_record ):
            {
                ipc::buffer::spsc_lock_free::init( channel );
            }
            break;
            default:
                assert( false );

        }

        
        channel->meta.type = type;
        if( ! ipc::buffer::channel_list_t::insert( &(data->buffer->data), 
                                                   channel_start,
                                                   &(data->buffer->channel_list) ) )
        {
            channel_start  = ipc::channel_err;
        }
    

    
    }
    /**
     * else, find_channel_buffer_offset function has set channel ptr to something valid
     * and the output contains the pointer to channel_info. 
     */
    //two directions on the channel, producer or consumer
    switch( dir )
    {
        case( ipc::producer ):
        {
            channel->meta.ref_count_prod++ /** atomic inc **/;
        }
        break;
        case( ipc::consumer ):
        {
            channel->meta.ref_count_cons++ /** atomic inc **/;
        }
        break;
        default:
        {
            //shouldn't be here
            assert( false );
        }
    }

POST:
    // Release semaphore
    /**
     * keep in mind, we jump here if we can't allocate memory too, 
     * so check ret channel pointer for null below before adding it
     * to the TLS. 
     */
    if( ipc::sem::post( sem ) == ipc::sem::uni_error )
    {
        ipc::buffer::gb_err.err_msg << 
            "Failed to post semaphore, exiting given we can't recover from this " << 
                "sem val(" << sem << ") @ line " << __LINE__;
        shutdown_handler( 0 );
    }
    
    if( channel != nullptr /** only case if we couldn't allocate mem for channel **/ )
    {
        /** insert zero count allocation struct into local calling TLS **/
        data->channel_local_allocation.insert( 
            std::make_pair( 
                channel_id, ipc::local_allocation_info( dir ) 
            ) 
        ); 
        /** insert channel structure into calling TLS **/
        data->channel_map.insert( std::make_pair( channel_id, channel ) );
    }
    return( channel_start );
}

ipc::channel_id_t
ipc::buffer::add_spsc_lf_record_channel(   ipc::thread_local_data   *data, 
                                           const channel_id_t       channel_id,
                                           ipc::direction_t         dir )
{
    return( ipc::buffer::add_channel( data, channel_id, ipc::spsc_record, dir ) );
}

std::size_t
ipc::buffer::get_record_size( ipc::thread_local_data *data, void *ptr )
{
    /** get number of blocks for the metadata **/
    auto data_block_offset  = calculate_block_offset( &data->buffer->data, ptr );
    
    const auto meta_multiple = 
        ipc::buffer::heap_t::get_block_multiple( sizeof( ipc::allocate_metadata ) );
   
    auto meta_offset = data_block_offset - meta_multiple;

    auto *meta_data = (ipc::allocate_metadata*)
        ipc::buffer::translate_block( &data->buffer->data, meta_offset);
    return( meta_data->block_count << ipc::block_size_power_two );  
}

//ipc::channel_id_t
//ipc::buffer::add_mn_lf_channel(   ipc::thread_local_data *data, 
//                            const channel_id_t channel_id )
//{
//    return( ipc::buffer::add_channel( data, channel_id, ipc::mpmc_record ) );
//}
//
    

bool
ipc::buffer::has_active_channels( ipc::thread_local_data *tls )
{
    return( ipc::buffer::channel_list_t::size( &tls->buffer->channel_list ) != 0 );  
}
    
bool
ipc::buffer::channel_has_producers( ipc::thread_local_data *tls, const ipc::channel_id_t channel )
{
    const auto found = tls->channel_map.find( channel );
    if( found == tls->channel_map.cend() )
    {
        return( false );
    }
    //else
    return( (*(*found).second).meta.ref_count_prod > 0 );
}
    

std::size_t 
ipc::buffer::channel_has_data( ipc::thread_local_data *data, const channel_id_t channel_id )
{
    const auto channel_found = data->channel_map.find( channel_id );
    if( channel_found == data->channel_map.cend() )
    {
        //FIXME - should we return some error code using negative spectrum? 
        return( 0 /** no data **/ );
    }
    auto *channel_info = (*channel_found).second;  
    std::size_t size = 0;
    switch( channel_info->meta.type )
    {
        case( ipc::mpmc_record):
        {
            assert( false /** unimplemented **/ );
            return( 0 );
        }
        break;
        case( ipc::spsc_record ):
        {
            size = ipc::buffer::spsc_lock_free::min_consumer_size( channel_info );
        }
        break;
        default:
            assert( false );

    }
    return( size );
}
    
ipc::channel_map_t
ipc::buffer::get_channel_list( ipc::thread_local_data *data )
{
    auto channel_map_output = ipc::make_channel_map();
    /** we don't want list changed out from under us **/
    auto sem = data->index_semaphore;
    if( ipc::sem::wait( sem ) == ipc::sem::uni_error )
    {
        ipc::buffer::gb_err.err_msg << 
            "Failed to wait, plz debug at line (" << __LINE__ << ")" << 
             " with sem value: " << sem;
        shutdown_handler( 0 );
    }
    //populate channel map 
    ipc::ptr_offset_t current_offset = ipc::invalid_ptr_offset;
    do
    {
        current_offset = 
        ipc::buffer::channel_list_t::get_next( current_offset, 
                                               &data->buffer->data,
                                               &data->buffer->channel_list );
        if( current_offset != ipc::invalid_ptr_offset )
        {
            auto *channel_index_struct = (ipc::channel_index_t*)
                                                translate_block( &data->buffer->data,
                                                                 current_offset );
            auto &ele = (**channel_index_struct);
            channel_map_output->insert( 
                std::make_pair( ele.meta.channel_id, 
                                ele.meta.type )
            );
        }                                                                 
    }while( current_offset != ipc::invalid_ptr_offset );

    /** release sem **/
    if( ipc::sem::post( sem ) == ipc::sem::uni_error )
    {
        ipc::buffer::gb_err.err_msg << 
            "Failed to post semaphore, exiting given we can't recover from this " << 
                "sem val(" << sem << ") @ line " << __LINE__;
        shutdown_handler( 0 );
    }
    return( channel_map_output );
}

ipc::channel_id_t
ipc::buffer::find_channel( ipc::thread_local_data *data,
                           const channel_id_t channel_id,
                           ipc::channel_info **channel )
{
    ipc::channel_id_t output = ipc::channel_not_found;
    /** acquire sem **/
    auto sem = data->index_semaphore;
    if( ipc::sem::wait( sem ) == ipc::sem::uni_error )
    {
        ipc::buffer::gb_err.err_msg << 
            "Failed to wait, plz debug at line (" << __LINE__ << ")" << 
             " with sem value: " << sem;
        shutdown_handler( 0 );
    }
                                      
    output = ipc::buffer::find_channel_buffer_offset( data, channel_id, channel );
    

    /** release sem **/
    if( ipc::sem::post( sem ) == ipc::sem::uni_error )
    {
        ipc::buffer::gb_err.err_msg << 
            "Failed to post semaphore, exiting given we can't recover from this " << 
                "sem val(" << sem << ") @ line " << __LINE__;
        shutdown_handler( 0 );
    }
    return( output );
}

bool
ipc::buffer::remove_channel( ipc::thread_local_data *data, 
                             const channel_id_t channel_id )
{
    /** acquire sem **/
    auto sem = data->index_semaphore;
    if( ipc::sem::wait( sem ) == ipc::sem::uni_error )
    {
        ipc::buffer::gb_err.err_msg << 
            "Failed to wait, plz debug at line (" << __LINE__ << ")" << 
             " with sem value: " << sem;
        shutdown_handler( 0 );
    }
    /**
     * get offset to the outter index structure, this is w.r.t. to blocks.
     */
    const auto channel_offset = ipc::buffer::find_channel_block( data, channel_id );
     
    if( channel_offset >= ipc::valid_offset )
    {
        
        /** remove the node **/
        ipc::buffer::channel_list_t::remove( &data->buffer->data,  
                                             channel_offset,
                                             &data->buffer->channel_list );
        
        auto *channel_index_struct = (ipc::channel_index_t*)
                                                translate_block( &data->buffer->data,
                                                                 channel_offset );
        /** finally, free channel entry itself **/
        ipc::buffer::free_record( data, channel_index_struct );
    }
    //chennel_offset should have an appropriate error message 
    
    /** release sem **/
    if( ipc::sem::post( sem ) == ipc::sem::uni_error )
    {
        ipc::buffer::gb_err.err_msg << 
            "Failed to post semaphore, exiting given we can't recover from this " << 
                "sem val(" << sem << ") @ line " << __LINE__;
        shutdown_handler( 0 );
    }

    return( channel_offset >= ipc::valid_offset );
}
   

void 
ipc::buffer::free_channel_memory( ipc::thread_local_data *data, ipc::local_allocation_info &info )
{
        /** free data that is previously there **/
        if( info.blocks_available > 0 )
        {
            //TODO - make sure this is actually going off of blocks vs. ptr offset, 
            //return blocks must be modulo block size.
            ipc::buffer::_free( data, 
                                info.local_allocation,
                                info.blocks_available ); 
        }
        info.blocks_available = 0;
        info.local_allocation = 0;
}

void
ipc::buffer::unlink_channel( ipc::thread_local_data *tls, ipc::channel_id_t channel )
{
    //get ptr for channel
    ipc::channel_info *ch_ptr = tls->channel_map[ channel ];
    //get rid of our local data allocation, return to buffer
    auto &th_local_allocation = tls->channel_local_allocation[ channel ]; 

    ipc::buffer::free_channel_memory( tls, th_local_allocation );
    /**
     * decrement refcount, if zero then free channel allocation, 
     * must get semaphore first.
     */

    /**
     * Acquire semaphore, must go after allocate otherwise we have 
     * nested semaphore acquire and deadlock.
     */
    auto sem = tls->index_semaphore;
    if( ipc::sem::wait( sem ) == ipc::sem::uni_error )
    {
        ipc::buffer::gb_err.err_msg << 
            "Failed to wait, plz debug at line (" << __LINE__ << ")" << 
             " with sem value: " << sem;
        shutdown_handler( 0 );
    }
    
    //two directions on the channel, producer or consumer
    switch( th_local_allocation.dir )
    {
        case( ipc::producer ):
        {
            //this part needs index sem
            --(ch_ptr->meta.ref_count_prod);
        }
        break;
        case( ipc::consumer ):
        {
            //this part needs index sem
            --(ch_ptr->meta.ref_count_cons);
        }
        break;
        default:
        {
            //shouldn't be here
            assert( false );
        }
    }


    /** release sem **/
    if( ipc::sem::post( sem ) == ipc::sem::uni_error )
    {
        ipc::buffer::gb_err.err_msg << 
            "Failed to post semaphore, exiting given we can't recover from this " << 
                "sem val(" << sem << ") @ line " << __LINE__;
        shutdown_handler( 0 );
    }
    
    if( ch_ptr->meta.ref_count_prod == 0 && ch_ptr->meta.ref_count_cons == 0 )
    {
        ipc::buffer::remove_channel( tls, channel );
    }
    tls->channel_map.erase( channel );
    tls->channel_local_allocation.erase( channel );
}


void
ipc::buffer::unlink_channels( ipc::thread_local_data *tls )
{
    for( auto ch_pair : tls->channel_map )
    {
        //get ptr for channel
        ipc::channel_info *ch_ptr = ch_pair.second;
        //get rid of our local data allocation, return to buffer
        auto &th_local_allocation = tls->channel_local_allocation[ 
            ch_pair.first /** channel id **/ ];

        ipc::buffer::free_channel_memory( tls, th_local_allocation );
        /**
         * decrement refcount, if zero then free channel allocation, 
         * must get semaphore first.
         */

        /**
         * Acquire semaphore, must go after allocate otherwise we have 
         * nested semaphore acquire and deadlock.
         */
        auto sem = tls->index_semaphore;
        if( ipc::sem::wait( sem ) == ipc::sem::uni_error )
        {
            ipc::buffer::gb_err.err_msg << 
                "Failed to wait, plz debug at line (" << __LINE__ << ")" << 
                 " with sem value: " << sem;
            shutdown_handler( 0 );
        }
    
        //two directions on the channel, producer or consumer
        switch( th_local_allocation.dir )
        {
            case( ipc::producer ):
            {
                //this part needs index sem
                --(ch_ptr->meta.ref_count_prod);
            }
            break;
            case( ipc::consumer ):
            {
                //this part needs index sem
                --(ch_ptr->meta.ref_count_cons);
            }
            break;
            default:
            {
                //shouldn't be here
                assert( false );
            }
        }


        /** release sem **/
        if( ipc::sem::post( sem ) == ipc::sem::uni_error )
        {
            ipc::buffer::gb_err.err_msg << 
                "Failed to post semaphore, exiting given we can't recover from this " << 
                    "sem val(" << sem << ") @ line " << __LINE__;
            shutdown_handler( 0 );
        }
        
        if( ch_ptr->meta.ref_count_prod == 0 && ch_ptr->meta.ref_count_cons == 0 )
        {
            ipc::buffer::remove_channel( tls, ch_pair.first );
        }
    }
    //clear map
    tls->channel_map.clear();
    tls->channel_local_allocation.clear();
}



void*
ipc::buffer::thread_local_allocate( 
                       ipc::thread_local_data *data,
                       const std::size_t blocks,
                       const channel_id_t channel_id )
{
    void *output = nullptr;
    auto info_iterator = data->channel_local_allocation.find( channel_id );
    if( info_iterator == data->channel_local_allocation.end() )
    {
        //FIXME - could return an error code here like mmap
        return( nullptr );
    }
    
    auto &local_allocation = (*info_iterator).second.local_allocation;
    auto &blocks_available = (*info_iterator).second.blocks_available;

    /**
     * we've gotten here, so we know we should at least have enough 
     * free memory to allocate the metadata structure + the requested
     * user-visible data blocks (that's the parameter given in the
     * function call.
     */
    ipc::buffer::create_meta_record( &data->buffer->data,
                                     local_allocation, 
                                     blocks_available,
                                     blocks );
    
    /**
     * get output pointer.
     */
    output = ipc::buffer::translate_block( &data->buffer->data, local_allocation ); 

    /**
     * now we need to go ahead an increment the local block count, not
     * ours anymore. 
     */
    local_allocation += blocks;
    blocks_available -= blocks;
    
    return( output );
}

void
ipc::buffer::create_meta_record( void                    *data_buffer_start,
                                 ipc::ptr_offset_t       &allocation_base,
                                 std::size_t             &blocks_avail,
                                 const std::size_t       multiple )
{
    //we're here, we should have enough local mem
    const auto meta_multiple = 
        ipc::buffer::heap_t::get_block_multiple( sizeof( ipc::allocate_metadata ) );
   

    void *meta_alloc    = ipc::buffer::translate_block( data_buffer_start,
                                                        allocation_base /** base offset **/ );
    assert( meta_alloc != nullptr );                                                      
    

    //blocks are guaranteed to be sequential
    allocation_base += meta_multiple;

    //make an actual meta-data structure
    new (meta_alloc) ipc::allocate_metadata( 
        allocation_base  /** help the allocator find the base and de-allocate this block **/,
        multiple         /** count of blocks allocated, not including this one **/
    );

    blocks_avail    -= meta_multiple; 
    return;
}


/** returns offset w.r.t. our buffer **/
ipc::ptr_offset_t
ipc::buffer::find_channel_buffer_offset( ipc::thread_local_data *data,
                           const channel_id_t channel_id,
                           ipc::channel_info **channel )
{
    assert( *channel    == nullptr );
    //this is the index node, not the channel info struct itself
    const auto found_node_offset( find_channel_block( data, channel_id ) );
    if( found_node_offset < ipc::valid_offset )
    {
        return( ipc::channel_not_found );
    }
    /** translate index  pointer **/
    auto *temp_index  = (ipc::channel_index_t*) ipc::buffer::translate_block( &data->buffer->data, found_node_offset );
    *channel = &(**temp_index);
    return( ipc::buffer::calculate_buffer_offset( &data->buffer->data, *channel ) );
}


/** returns block **/
ipc::ptr_offset_t
ipc::buffer::find_channel_block( ipc::thread_local_data *data,
                                  const channel_id_t      channel_id )
{
    assert( data        != nullptr );
    ipc::channel_index_t temp_node( channel_id            /** node_id **/,
                                    ipc::nodebase::normal /** node type **/,
                                    channel_id );
    //initialize to null effectively

    ipc::ptr_offset_t found_node_offset( ipc::ptr_not_found );
    
    found_node_offset = 
        ipc::buffer::channel_list_t::find( &(data->buffer->data), 
                                           temp_node, 
                                           &(data->buffer->channel_list) );
    return( found_node_offset );
}

void*
ipc::buffer::global_buffer_allocate( ipc::thread_local_data *data, 
                                     std::size_t            &blocks_needed,
                                     bool                   force_size )
{
    /**
     * if we're here we need to go out to the global buffer,
     * then the calling function can call the thread local
     * allocate function.
     */
    /**
     * STEP 0: either allocate a multiple of our standard block size
     * or if the nbytes requested is bigger, try to allocate something
     * larger, as in 3x block size. 
     */
    const auto blocks_for_data = blocks_needed * 3;
    
    const auto large_block_multiple = 
        ipc::buffer::heap_t::get_block_multiple( ipc::buffer::global_block_inc );
   
    /**
     * STEP 1: calculate the multiple of increment size to allocate here.
     */
    const std::int64_t  block_usage_estimate = 
        (blocks_for_data > large_block_multiple ? blocks_for_data : large_block_multiple );
    
    /**
     * give the option to force the size to exactly the multiple specified
     * and just assign it here.
     */
    blocks_needed = ( force_size ? blocks_needed : block_usage_estimate );


    /**
     * STEP 2: initialize output pointer.
     */
    void *output = nullptr;
    
    //get thread local version of allocate semaphore
    /** LOCK **/
    auto sem = data->allocate_semaphore;
    if( ipc::sem::wait( sem ) == ipc::sem::uni_error )
    {
        ipc::buffer::gb_err.err_msg << 
            "Failed to wait, plz debug at line (" << __LINE__ << ")" << 
             " with sem value: " << sem;
        shutdown_handler( 0 );
    }

    if( blocks_needed <= ipc::meta_info::heap_t::get_blocks_avail( &data->buffer->heap  ) )
    {
        /**
         * STEP 3: set start pointer to the current free start
         */
        const auto block_base = ipc::meta_info::heap_t::get_n_blocks( blocks_needed,
                                                                      &data->buffer->heap ); 
        output = ipc::buffer::translate_block( (void *) &data->buffer->data,
                                               block_base /** base offset **/ );
    }
    //if not the right size, go to sem_post and we end up here. 
    
    /** UNLOCK **/
    if( ipc::sem::post( sem ) == ipc::sem::uni_error )
    {
        ipc::buffer::gb_err.err_msg << 
            "Failed to post semaphore, exiting given we can't recover from this " << 
                "sem val(" << sem << ") @ line " << __LINE__;
        shutdown_handler( 0 );
    }
    return( output /** null or valid pointer **/);
}





void*
ipc::buffer::allocate_record( ipc::thread_local_data *data,
                       const std::size_t      nbytes,
                       const ipc::channel_id_t channel_id )
{
    assert( data != nullptr );
    
    /** check that channel exists for this tls structure **/ 
    auto channel_found = data->channel_local_allocation.find( channel_id );
    if( channel_found == data->channel_local_allocation.end() )
    {
        return( nullptr );
    }


    //check minimum bytes for single allocation, requested + meta
    const std::size_t meta_blocks  = 
        heap_t::get_block_multiple( sizeof( ipc::allocate_metadata ) );
    
    std::size_t data_blocks  = 
        heap_t::get_block_multiple( nbytes );

    auto &th_local_allocation = (*channel_found).second;


    if( (meta_blocks + data_blocks) <= th_local_allocation.blocks_available /** check thread local buffer **/)
    {
        //allocate from local buffer, this func calls meta builder func
        return( 
            ipc::buffer::thread_local_allocate(
                data, data_blocks, channel_id
            )
        );
    }
    /** free data that is previously there **/
    
    ipc::buffer::free_channel_memory( data, th_local_allocation );

    /** else allocate from global buffer **/
    std::size_t global_blocks_allocated = data_blocks;;
    auto *ret_ptr = ipc::buffer::global_buffer_allocate( data, 
                                                         global_blocks_allocated );
    if( ret_ptr == nullptr )
    {
        //global buffer allocate failed
        return( nullptr );
    }
    
    
    th_local_allocation.local_allocation = calculate_block_offset( &data->buffer->data, 
                                                                   ret_ptr ); 
    th_local_allocation.blocks_available = global_blocks_allocated;
    /**
     * call normal allocate given we now have memory to give 
     * back, this will set up the meta header. 
     */
    return( 
        ipc::buffer::thread_local_allocate(
            data, data_blocks, channel_id
        ) );
    return( nullptr );
}


void
ipc::buffer::_free(   ipc::thread_local_data *data,
                      const ipc::ptr_offset_t block_base,
                      const std::size_t       blocks )
{
    /** LOCK **/
    auto sem = data->allocate_semaphore;
    if( ipc::sem::wait( sem ) == ipc::sem::uni_error )
    {
        ipc::buffer::gb_err.err_msg << 
            "Failed to wait, plz debug at line (" << __LINE__ << ")" << 
             " with sem value: " << sem;
        shutdown_handler( 0 );
    }
    
    /**
     * FIXME - need to formalize the allocate vs. send policy. 
     * we want to be able to allocate thread local blocks while
     * being able to allocate locally and still send records without
     * having to re-acquire the semaphore.
     */
    heap_t::return_n_blocks( block_base, 
                             blocks,
                             &data->buffer->heap );
   
    //HERE
    
    /** UNLOCK **/
    if( ipc::sem::post( sem ) == ipc::sem::uni_error )
    {
        ipc::buffer::gb_err.err_msg << 
            "Failed to post semaphore, exiting given we can't recover from this " << 
                "sem val(" << sem << ") @ line " << __LINE__;
        shutdown_handler( 0 );
    }

}


void
ipc::buffer::free_record(   ipc::thread_local_data *data,
                            void *ptr )
{
    
    /** get number of blocks for the metadata **/
    auto data_block_offset  = calculate_block_offset( &data->buffer->data, ptr );
    
    const auto meta_multiple = 
        ipc::buffer::heap_t::get_block_multiple( sizeof( ipc::allocate_metadata ) );
   
    auto meta_offset = data_block_offset - meta_multiple;

    auto *meta_data = (ipc::allocate_metadata*)
        ipc::buffer::translate_block( &data->buffer->data, meta_offset);
    
    const auto blocks_to_free = meta_multiple + meta_data->block_count;
    
    
    ipc::buffer::_free( data, 
                        meta_offset,
                        blocks_to_free );

    return;
}


ipc::tx_code
ipc::buffer::send_record( ipc::thread_local_data *tls_data,
                          const ipc::channel_id_t channel_id,
                          void **record )
{
    //FIXME - need to check record for meta info region 
    //at block - 1.
    assert( tls_data != nullptr );
    /** check that channel exists for this tls structure **/ 
    auto channel_found = tls_data->channel_map.find( channel_id );
    if( channel_found == tls_data->channel_map.end() )
    {
        return( ipc::no_such_channel );
    }
    //get channel structure
    auto *channel_info = (*channel_found).second;  
    

    ipc::tx_code ret_code = tx_error;
    switch( channel_info->meta.type )
    {
        case( ipc::mpmc_record):
        {
            //allocate a record object, doesn't matter if it's local or global
            auto *mem_for_record_node = 
                allocate_record( tls_data, 
                                 sizeof( ipc::record_index_t ), 
                                 channel_id );
            auto *record_node = 
                 new (mem_for_record_node) ipc::record_index_t( ipc::nodebase::normal ); 
            /**
             * FIXME - might need to check the return code to see if the block offset
             * is within range.
             */
            (**record_node) = 
                ipc::buffer::calculate_block_offset( &tls_data->buffer->data, *record ); 
            /** 
             * technically this always succeeds at the moment..unless 
             * something bad happens. 
             */
            ret_code = 
                ipc::buffer::mpmc_lock_free::push( channel_info, 
                                                   record_node, 
                                                   &tls_data->buffer->data ); 
        }
        break;
        case( ipc::spsc_record ):
        {
            /**
             * the mpmc one we have to make a lock-free node, but for this
             * ringbuffer, we just put in the raw pointer. 
             */
            ret_code = 
                ipc::buffer::spsc_lock_free::push( channel_info, 
                                                   *record, 
                                                   &tls_data->buffer->data ); 
        }
        break;
        default:
            assert( false );

    }
    //if success then record node and record no longer belong to us.
    return( ret_code ); 
}

ipc::tx_code
ipc::buffer::receive_record( ipc::thread_local_data *tls_data,
                             const ipc::channel_id_t channel_id,
                             void **record )
{
    assert( tls_data != nullptr );
    *record = nullptr;    
    /** check that channel exists for this tls structure **/ 
    auto channel_found = tls_data->channel_map.find( channel_id );
    if( channel_found == tls_data->channel_map.end() )
    {
        return( ipc::no_such_channel );
    }
    auto *channel_info = (*channel_found).second;  
    
    ipc::tx_code ret_code = ipc::tx_success; 
    
    switch( channel_info->meta.type )
    {
        case( ipc::mpmc_record ):
        {
            //we'll receive data here, will need to unpact from queue node
            ipc::record_index_t *ptr = nullptr;

            ret_code = 
                ipc::buffer::mpmc_lock_free::pop( channel_info, &ptr, &tls_data->buffer->data );
            
            //need to check ret code
            if( ret_code != tx_success )
            {
                //return a correct error code of our own, likely just retransmit
                //record is already nullptr
                return( ret_code );
            }
            //else
            *record = 
                ipc::buffer::translate_block( &tls_data->buffer->data, (**ptr ) );
            //sanity check
            assert( *record != nullptr );
            //already checked ptr as non-null, free the record node.
            ipc::buffer::free_record( tls_data, ptr );
        }
        break;
        case( ipc::spsc_record ):
        {
            /**
             * spsc ringbuffer returns pointer directly, no need
             * to do much else, just return the ret_code
             * back to the caller. 
             */
            ret_code = 
                ipc::buffer::spsc_lock_free::pop( channel_info, 
                                                  record, 
                                                  &tls_data->buffer->data );
        }
        break;
        default:
            assert( false );

    }
    return( ret_code ); 
}



ipc::thread_local_data*
ipc::buffer::get_tls_structure( ipc::buffer *buffer,
                                const ipc::thread_id_t thread_id )
{
    ipc::thread_local_data *ptr = nullptr;
    //called in the context of the calling thread ID
    ptr = new thread_local_data();
    errno = 0;
    
    

    ptr->index_semaphore    = ipc::sem::open( 
        buffer->index_sem_name,
        0x0,
        ipc::sem::file_rdwr );
    /**
     * Open semaphore.
     */
    if( ptr->index_semaphore == ipc::sem::sem_error )
    {
        ipc::buffer::gb_err.err_msg << "Failed to open index semaphore!!";
        shutdown_handler( 0 );
    }

    ptr->allocate_semaphore = ipc::sem::open( 
        buffer->alloc_sem_name,
        0x0,
        ipc::sem::file_rdwr );

    if( ptr->allocate_semaphore == ipc::sem::sem_error )
    {
        ipc::buffer::gb_err.err_msg << "Failed to open allocate semaphore!!";
        shutdown_handler( 0 );
    }

    //call subinit function
    if( ipc::sem::sub_init( ptr->allocate_semaphore ) == -1 )
    {
        ipc::buffer::gb_err.err_msg << "failed to initialize semaphore"; 
        shutdown_handler( 0 );
    }
    if( ipc::sem::sub_init( ptr->index_semaphore ) == -1 )
    {
        ipc::buffer::gb_err.err_msg << "failed to initialize index semaphore"; 
        shutdown_handler( 0 );
    }

    // tls is now allocated (and semaphores open), now register this thread in the index
    ptr->buffer         = buffer;
    ptr->thread_id      = thread_id;
   

    return( ptr );
}

void
ipc::buffer::close_tls_structure( ipc::thread_local_data* data )
{
    ipc::buffer::unlink_channels( data );
    // Close semaphores
    ipc::sem::close( data->allocate_semaphore );
    ipc::sem::close( data->index_semaphore    );
    
    ::free( data );
    return;
}
