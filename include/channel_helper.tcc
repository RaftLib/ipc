/**
 * channel_helper.tcc - 
 * @author: Jonathan Beard
 * @version: Mon Jan 31 13:15:55 2022
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
#ifndef CHANNEL_HELPER_TCC
#define CHANNEL_HELPER_TCC  1
    
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
#include "buffer_helper.tcc"


/**
 * You'll have to do something interesting here, you need to "inject"
 * the class from buffer to here via template so that this works. It 
 * has to be a non-member template so we can do specialization, but 
 * it also needs access to all the local functions too. Likely has
 * to be a "friend" too. 
 */

namespace ipc
{

template < ipc::channel_type ct, 
           std::size_t n_entries = 0, 
           class T = void > struct channel_helper
{
    static ipc::channel_id_t add_channel(  ipc::thread_local_data                *tls,
                                           const channel_id_t                    channel_id,
                                           const ipc::direction_t                dir,
                                        ipc::buffer::shm_seg::init_func_t     f   = nullptr )
    {
    
        return( ipc::channel_err );
    }
};

/**
 * specialization for shared memory block type
 */
template <> struct channel_helper< ipc::shared, 0, void >
{
     static ipc::channel_id_t add_channel(  ipc::thread_local_data                *tls,
                                            const channel_id_t                    channel_id,
                                            const ipc::direction_t                dir,
                                        ipc::buffer::shm_seg::init_func_t     f   = nullptr )
     {

         ipc::buffer::shm_seg::data data;
         const auto seg_base = channel_start + channel_info_multiple; 
         auto *seg_ptr = 
             ipc::buffer::translate_block( &data->buffer->data, seg_base );
         
         data.buffer_base = &tls->buffer->data;
         data.seg_base    = seg_ptr;
         data.f           = f;
         const auto channel_ret_val =  
         ipc::channel_helper_common<
                                      ipc::shared,
                                      0,
                                      void,
                                      ipc::channel_info_record,
                                      ipc::shared_seg >::channel_adder( tls, 
                                                                        channel_id, 
                                                                        dir, 
                                                                        &data );
 
        if( channel_ret_val == false /** channel creation failed **/ )
        {
            return( ipc::channel_err );
        }
        else
        {
            return( channel_id );
        }
     }
 
};


/**
 * specialization for spsc_record type
 */
template <> struct channel_helper< ipc::spsc_record, 0, void >
{
     static ipc::channel_id_t add_channel(  ipc::thread_local_data                *tls,
                                            const channel_id_t                    channel_id,
                                            const ipc::direction_t                dir )
     {

        
         const auto channel_ret_val =  
         ipc::channel_helper_common< 
                    ipc::spsc_record,
                    0,
                    void,
                    ipc::channel_info_record,
                    ipc::spsc_lock_free >::channel_adder( tls, 
                                                          channel_id, 
                                                          dir, 
                                                          nullptr );
 
        if( channel_ret_val == false /** channel creation failed **/ )
        {
            return( ipc::channel_err );
        }
        else
        {
            return( channel_id );
        }
     }
 
};


/**
 * specialization for spsc_record type
 */
template <> struct channel_helper< ipc::mpmc_record, 0, void >
{
     static ipc::channel_id_t add_channel(  ipc::thread_local_data                *tls,
                                            const channel_id_t                    channel_id,
                                            const ipc::direction_t                dir )
     {
         //set up dummy node for LF queue 
         auto dummy_info_multiple  = 
             ipc::buffer::heap_t::get_block_multiple( 
                                 sizeof( ipc::record_index_t ) );
         
         void *mem_for_dummy_node = 
             ipc::buffer::global_buffer_allocate(  data, 
                                                   dummy_info_multiple,
                                                   true ); 

         auto *dummy_record = 
             new (mem_for_dummy_node) ipc::record_index_t( ipc::nodebase::dummy );
         

         channel->meta.dummy_node_offset = 
             ipc::buffer::calculate_block_offset( &data->buffer->data, 
                                                  dummy_record );
         ipc::mpmc_lock_free::data d;
         d.buffer_base = &data->buffer-data;
         d.dummy_node  = dummy_record;

         
         const auto channel_ret_val =  
         ipc::channel_helper_common< 
                        ipc::mpmc_record,
                        0,
                        void,
                        ipc::channel_info_record,
                        ipc::mpmc_lock_free >::channel_adder( tls, channel_id, dir, &data );
 
        if( channel_ret_val == false /** channel creation failed **/ )
        {
            //deallocate dummy node, abort, return error 
            ipc::_free( tls,
                        channel->meta.dummy_node_offset,
                        dummy_info_multiple );
            return( ipc::channel_err );
        }
        else
        {
            return( channel_id );
        }
    }         
};

} /** end namespace ipc **/
#endif /* END CHANNEL_HELPER_TCC */
