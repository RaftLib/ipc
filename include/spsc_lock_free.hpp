/**
 * spsc_lock_free.hpp - 
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
#ifndef SPSC_LOCK_FREE_HPP
#define SPSC_LOCK_FREE_HPP  1
#include "bufferdefs.hpp"
#include "ch_entries_spsc.hpp"
#include <cassert>

namespace ipc
{

template < class PARENTNODE, class LOCKFREE_NODE, class TRANSLATE > 
    class spsc_lock_free_queue
{
private:
    
    spsc_lock_free_queue() = delete;
    
    using self_t = spsc_lock_free_queue< PARENTNODE, LOCKFREE_NODE, TRANSLATE >;
    
    /**
     * inc
     */
    inline 
    static 
    void
    inc_tail( PARENTNODE *channel ) 
    {
       auto &ptr = channel->ctrl_all.data_tail;
       ptr = ( ptr + 1 ) % ipc::ch_entries_spsc::n_entries;
       if( ptr == 0 )
       {
          channel->ctrl_spsc.wrap_tail++;
       }
    }
    
    /**
     * inc
     */
    inline 
    static 
    void
    inc_head( PARENTNODE *channel ) 
    {
       auto &ptr = channel->ctrl_all.data_head;
       ptr = ( ptr + 1 ) % ipc::ch_entries_spsc::n_entries;
       if( ptr == 0 )
       {
          channel->ctrl_spsc.wrap_head++;
       }
    }

public:
    inline static std::size_t size( PARENTNODE *channel )
    {
        for( ;; )
        {
TOP:        
            const auto   wrap_write = 
                channel->ctrl_spsc.wrap_tail.load( std::memory_order_relaxed );
            const auto   wrap_read  = 
                channel->ctrl_spsc.wrap_head.load( std::memory_order_relaxed );
            const auto   wpt = 
                channel->ctrl_all.data_tail.load( std::memory_order_relaxed );
            const auto   rpt = 
                channel->ctrl_all.data_head.load( std::memory_order_relaxed );

                   
            if( wpt == rpt)
            {
               /** expect most of the time to be full **/
               if(  wrap_read < wrap_write )
               {
                  return( ipc::ch_entries_spsc::n_entries );
               }
               else if( wrap_read > wrap_write )
               {
                  goto TOP;
               }
               else
               {
                  return( 0 );
               }
            }
            else if( rpt < wpt )
            {
               return( wpt - rpt );
            }
            else if( rpt > wpt )
            {
               return( ipc::ch_entries_spsc::n_entries - rpt + wpt ); 
            }
            return( 0 );
        } /** end for **/
        return( 0 ); /** keep some compilers happy **/
    }
   
   /**
    * space_avail - returns the amount of space currently
    * available in the queue.  This is the amount a user
    * can expect to write without blocking
    * @return  size_t
    */
   inline static std::size_t space_avail( PARENTNODE *channel )
   {
      return( ipc::ch_entries_spsc::n_entries - self_t::size( channel ) );
   }
    

   

    
    /**
     * init -  
     * @return - void.
     */
    static void init( PARENTNODE *channel )
    {
        /**
         * different initialization condition than the mpmc
         * queue.
         */
        channel->ctrl_all.data_head = 
            channel->ctrl_all.data_tail = 0;
        /**
         * initialize the credit count for producer to total available
         */
        channel->meta.prod_credits = ipc::ch_entries_spsc::n_entries;
        //consumer credits already zero. 
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
    inline static ipc::tx_code push( PARENTNODE *channel, 
                              LOCKFREE_NODE *node_to_add, 
                              void *buffer_base )
    {
        //check credits on tail
PUSH_RETRY:
        if( channel->meta.prod_credits > 0 )
        {   
            const auto offset_to_add = 
                TRANSLATE::calculate_block_offset( buffer_base,
                                                   node_to_add );
            channel->spsc_q.entry[ 
                channel->ctrl_all.data_tail
            ] = offset_to_add;
            inc_tail( channel );
            channel->meta.prod_credits--;
            node_to_add = nullptr;
            return( ipc::tx_success );
        }
        //else if credits == 0
        else
        {
            const auto curr_space = self_t::space_avail( channel );
            if( curr_space == 0 )
            {
                return( ipc::tx_retry );
            }
            else
            {
                channel->meta.prod_credits = curr_space;
                goto PUSH_RETRY;
            }
        }
        return( ipc::tx_success );
    }

    /**
     * pop - pop a node from the ring buffer
     */
    inline static ipc::tx_code pop( PARENTNODE *channel, 
                             LOCKFREE_NODE **receive_node, 
                             void *buffer_base )
    {
        if( channel->meta.cons_credits == 0 )
        {
            channel->meta.cons_credits = self_t::size( channel );
            return( ipc::tx_retry );
        }
        //else 
        const auto offset = channel->spsc_q.entry[ channel->ctrl_all.data_head ];

        *receive_node =
            (LOCKFREE_NODE*) TRANSLATE::translate_block( buffer_base, offset );

        self_t::inc_head( channel );

        //decrement credits
        channel->meta.cons_credits--;
        return( ipc::tx_success );
    }

private:

}; /** end class spsc_lock_free_queue **/

} /** end namespce ipc **/

#endif /* ENdf;lkajsd;flkj SPSC_LOCK_FREE_HPP */
