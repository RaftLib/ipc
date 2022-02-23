/**
 * buffer_helper.tcc - 
 * @author: Jonathan Beard
 * @version: Mon Jan 31 13:13:17 2022
 * 
 * Copyright 2022 Jonathan Beard
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
#ifndef BUFFER_HELPER_TCC
#define BUFFER_HELPER_TCC  1

#include <cstdint>
#include <string>
#include <sstream>
#include <functional>


#include "genericnode.hpp"
#include "indexbase.hpp"
#include "database.hpp"
#include "threadlocaldata.hpp"
#include "bufferdefs.hpp"
#include "meta_info.hpp"

namespace ipc
{

/** 
 * a bit ugly, but, channel_helper allows specialization
 * based on the template parameters which in turn simplifies
 * the construction here.
 */
struct channel_helper_common
{
    template< ipc::channel_type ct,
              std::size_t n_entries,
              class CHANNEL_DATA_TYPE,
              class CHANNEL_INFO_TYPE,
              class CHANNEL_TRANSPORT_TYPE
              >
    static bool              channel_adder( ipc::thread_local_data *tls,
                                            const channel_id_t      channel_id,
                                            const ipc::direction_t  dir,
                                            const std::size_t       additional_bytes,
                                            void                    *data )
    {
        using CHANNEL_INDEX_TYPE = ipc::index_t< CHANNEL_INFO_TYPE >;
        assert( tls != nullptr );    
        
        /** 
         * channel is typed here, we cast to one of the base-classes
         * later so helper functions can have access to a common set
         * of fields. 
         */
        CHANNEL_INFO_TYPE   *channel    = nullptr;
        ipc::channel_id_t channel_start = ipc::null_channel;
        

        const auto channel_info_multiple  = 
            ipc::meta_info::heap_t::get_block_multiple( sizeof( CHANNEL_INDEX_TYPE ) );
        //this one is always the same unless we change it at some point
        const auto meta_multiple = 
            ipc::meta_info::heap_t::get_block_multiple( sizeof( ipc::allocate_metadata ) );
        
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
        
        /**
         * check to see if the channel already exists, if it does, then 
         * channel start points to a block offset that is modulo block_size
         */
        channel_start  = 
            ipc::buffer::find_channel_buffer_offset( tls, 
                                                     channel_id, 
                                                     (ipc::channel_info**)&channel );
        if( channel_start < ipc::valid_offset )
        {
            const auto additional_byte_multiple = 
                    ipc::buffer::heap_t::get_block_multiple( additional_bytes );

            //add up everything with a *_multiple
            const std::size_t blocks_to_allocate = 
                channel_info_multiple + meta_multiple + additional_byte_multiple;
            
            // Create new node -- will have to acquire allocation semaphore
            // BEWARE - we already grabbed index semaphore, they're now nested
            void *mem_for_new_channel = 
                ipc::buffer::global_buffer_allocate(  tls, 
                                                      blocks_to_allocate,
                                                      true ); 

            if( mem_for_new_channel == nullptr )
            {
                channel_start = ipc::channel_alloc_err;
                //jump to semaphore exit
                goto POST;
            }
            
            const auto meta_offset = 
                ipc::buffer::calculate_block_offset( &tls->buffer->data, 
                                                     mem_for_new_channel );


            /** need to calculate channel start, don't change pointer yet, we need that **/
            channel_start = meta_offset + meta_multiple;

            //make an actual meta-data structure
            new (mem_for_new_channel) ipc::allocate_metadata( 
                channel_start           
                /** help the allocator find the base and de-allocate this block **/,
                channel_info_multiple + additional_byte_multiple  
                /** count of blocks allocated, not including metadata **/
            );
            
            
            mem_for_new_channel = 
                ipc::buffer::translate_block( &tls->buffer->data, channel_start );

            //initialize new channel structure
            auto *node_to_add = new (mem_for_new_channel)
                CHANNEL_INDEX_TYPE( channel_id            /** id **/,
                                    ipc::nodebase::normal /** node type **/,
                                    channel_id );
            /**
             * this gives you a pointer back to the first block that is not the "meta" node
             */
            channel            = (CHANNEL_INFO_TYPE*)&(**node_to_add);
            channel->meta.type = ipc::spsc_record;
            

            /**
             * #####
             * ## THIS IS WHERE YOU'RE ACTUALLY INITIALIZING
             * ## YOUR DATA STRUCTURES...MAKE SURE YOU HAVE
             * ## THE RIGHT KIND OF DATA FOR THE CHANNEL TYPE
             * ## OTHERWISE BAD THINGS MAY HAPPEN. 
             */
            CHANNEL_TRANSPORT_TYPE::init( channel, data );
            
            if( ! ipc::buffer::channel_list_t::insert( &(tls->buffer->data), 
                                                       channel_start,
                                                       &(tls->buffer->channel_list) ) )
            {
                channel_start  = ipc::channel_err;
            }
        } //end of channel creation if statement
        
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
            case( ipc::dir_not_set ):
            {
                if( channel->meta.type == ipc::shared )
                {
                    channel->meta.ref_count_shd++ /** atomic inc **/;
                }
            }
            break;
            default:
            {
                //shouldn't be here
                assert( false );
            }
        }
:
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
            tls->channel_local_allocation.insert( 
                std::make_pair( 
                    channel_id, ipc::local_allocation_info( dir ) 
                ) 
            ); 
            /** insert channel structure into calling TLS **/
            tls->channel_map.insert( std::make_pair( channel_id, channel ) );
        }
        if( channel_start == ipc::channel_alloc_err || channel_start < ipc::valid_offset )
        {
            return( false );
        }
        else
        {
            return( true );
        }
    } //end of 'channel_adder' function

};

} /** ened namespace helper **/
#endif /* END BUFFER_HELPER_TCC */
