/**
 * lock_free.hpp - 
 * @author: Jonathan Beard
 * @version: Wed Jul 28 12:50:28 2021
 * 
 * Copyright 2021 Jonathan Beard
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
#ifndef LOCK_FREE_HPP
#define LOCK_FREE_HPP  1
#include "bufferdefs.hpp"
#include <cassert>
#include <atomic>

namespace ipc
{

template < class PARENTNODE, class LOCKFREE_NODE, class TRANSLATE > 
    class mpmc_lock_free_queue
{
public:
    using self_t = mpmc_lock_free_queue< PARENTNODE, LOCKFREE_NODE, TRANSLATE >;
   
    mpmc_lock_free_queue() = delete;

    
    /**
     * init - for the lock free queue, the "meta" is the control region. For 
     * an example, the meta is the channelinfo struct which contains a few
     * things that matter to the lock free queue, specifically the head
     * pointer, tail pointer, and a dummy node offset. This dummy node for
     * the channel will be of type record_index_t, however, the control
     * region will only contain a pointer to the offset.
     * @param meta - the lock-free channel header info.
     * @param dummy - dummy node of whichever type of lock-free queue you've made
     * this dummy should extend "genericnode" and be allocated in the outter
     * buffer shared memory segment. 
     * @param buffer - base of buffer address where all the offsets are relative
     * to, the class template provides a translate class that has static methods
     * that should allow this LF queue to translate buffer offsets into the 
     * calling VA space. 
     * @return - void.
     */
    static void init( PARENTNODE *channel, LOCKFREE_NODE *dummy, void *buffer_base )
    {
        //get dummy node offset for this channel (to be used by all endpoints)
        const auto node_block_address = TRANSLATE::calculate_block_offset( buffer_base, dummy );
        //set all offsets initially to dummy node
        channel->ctrl_all.data_head = 
            channel->ctrl_all.data_tail = 
                channel->meta.dummy_node_offset = node_block_address;
        return;
    }

    /**
     * push - push a node into the LF queue.
     * @param meta - the lock-free channel header info.
     * @param node_to_add - node of whichever type of lock-free queue you've made
     * this should extend "genericnode" and be allocated in the outter
     * buffer shared memory segment. 
     * @param buffer - base of buffer address where all the offsets are relative
     * to, the class template provides a translate class that has static methods
     * that should allow this LF queue to translate buffer offsets into the 
     * calling VA space. 
     */
    static ipc::tx_code push( PARENTNODE *channel,  
                              LOCKFREE_NODE *node_to_add, 
                              void *buffer_base )
    {
        /**
         * simple sanity checks
         */
        assert( channel         != nullptr );
        assert( node_to_add     != nullptr );
        assert( buffer_base     != nullptr );
        /** 
         * channel is the pointer with respect to the callers VA space,
         * the pointers contained within though, are block aligned
         * modulo the global block_size, so, buffer offset modulo block
         * size.
         */

         /** 
          * node to add should be same VA in callers address space, 
          */
            
        bool success( false );

        node_to_add->prev = channel->ctrl_all.data_tail;
        
        const auto node_block_address = 
            TRANSLATE::calculate_block_offset( buffer_base, node_to_add );

        while( !success )
        {
            /** swing our block value in as tail **/
            success = 
             channel->ctrl_all.data_tail.compare_exchange_strong( 
                         node_to_add->prev  /** expected ref **/,
                         node_block_address /** desired value **/,
                         std::memory_order_release /** memory order success **/,
                         std::memory_order_relaxed /** memory order failure **/ );

        }
        //get address 
        LOCKFREE_NODE *prev = 
            (LOCKFREE_NODE*) TRANSLATE::translate_block( buffer_base, 
                                                         node_to_add->prev );

        prev->next.store( node_block_address, std::memory_order_relaxed );

        return( ipc::tx_success );
    }

    /**
     * pop - pop a node into the LF queue.
     * @param meta - the lock-free channel header info.
     * @param receive_node - node of whichever type of lock-free queue you've made
     * this should extend "genericnode" and be allocated in the outter
     * buffer shared memory segment. Once you get a pointer successfully with pop,
     * this is your pointer, and you must free it back to the buffer when you're ready 
     * to get rid of it. 
     * @param buffer - base of buffer address where all the offsets are relative
     * to, the class template provides a translate class that has static methods
     * that should allow this LF queue to translate buffer offsets into the 
     * calling VA space. 
     */
    static ipc::tx_code pop( PARENTNODE *channel, 
                             LOCKFREE_NODE **receive_node, 
                             void *buffer_base )
    {
        *receive_node = nullptr;
        /**
         * get these atomically, might just need a fence, 
         * but...dies on all arch with -O2 and above. 
         */
        ipc::ptr_offset_t local_head_offset    = ipc::invalid_ptr_offset;
        ipc::ptr_offset_t local_tail_offset    = ipc::invalid_ptr_offset;



        //load up current head into a local copy
        local_head_offset = 
            channel->ctrl_all.data_head.load( std::memory_order_relaxed );

        //translate local head to an actual address
        auto *local_head_ptr = 
            (LOCKFREE_NODE*) TRANSLATE::translate_block( buffer_base, 
                                                         local_head_offset );
        auto local_next_offset = 
            local_head_ptr->next.load( std::memory_order_relaxed );
        
        local_tail_offset = 
            channel->ctrl_all.data_tail.load( std::memory_order_relaxed );


        
        if( local_head_offset == local_tail_offset )
        {
            if( local_head_ptr->_type == ipc::nodebase::dummy )
            {
                return( ipc::tx_retry );
            }
            else
            {
                //single node, non-dummy, let's add one so we can proceed
                auto *dummy_ptr = (LOCKFREE_NODE*) TRANSLATE::translate_block( buffer_base, 
                                                                               channel->meta.dummy_node_offset );
                self_t::push( channel, dummy_ptr, buffer_base );
                return( ipc::tx_retry );
            }
        }
        else if( local_next_offset == ipc::nodebase::init_offset() )
        {
            
            //head != tail, but still no local_next...
            //safe thing is to wait and come around again
            return( ipc::tx_retry );
        }
        else
        {
            /** swing our block value in as tail **/
            const bool
            success = 
             channel->ctrl_all.data_head.compare_exchange_weak( 
                         local_head_offset  /** expected ref **/,
                         local_next_offset  /** desired value **/,
                         std::memory_order_release /** memory order success **/,
                         std::memory_order_relaxed /** memory order failure **/ );
            if( success )
            {
                auto *success_local_head_ptr = 
                    (LOCKFREE_NODE*) TRANSLATE::translate_block( buffer_base, 
                                                                 local_head_offset );
                /**
                 * no need to go back and get the channel offset, if it's 
                 * the dummy, there's only one, push it. 
                 */
                if( success_local_head_ptr->_type == ipc::nodebase::dummy )
                {
                    self_t::push( channel, success_local_head_ptr, buffer_base );
                    return( ipc::tx_retry );
                }
                else
                {
                    *receive_node =  success_local_head_ptr;
                    return( ipc::tx_success );
                }
            }
            else
            {
                return( ipc::tx_retry );
            }
        }
        return( ipc::tx_retry );

    }

private:

}; /** end class mpmc_lock_free_queue **/

} /** end namespce ipc **/

#endif /* END LOCK_FREE_HPP */
