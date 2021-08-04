/**
 * lock_ll.hpp - simple routines that enable construction of a linked
 * list within a shared memory segment. Instead of using direct VAs, this
 * class uses offsets which are converted to virtual addresses within the 
 * class. 
 * @author: Jonathan Beard
 * @version: Thu Jul  8 06:41:07 2021
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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef LOCK_LL_HPP
#define LOCK_LL_HPP  1
#include <cstdint>
#include <climits>
#include "genericnode.hpp"
#include "bufferdefs.hpp"
#include <iostream>

namespace ipc
{

template < class NODE_TYPE, class TRANSLATE > class lock_ll
{
public:
    lock_ll()   = default;
    ~lock_ll()  = default;

    using self_t = lock_ll< NODE_TYPE, TRANSLATE >;

   /**
    * what we need...
    * linked_list.insert
    * linked_list.remove
    * linked_list.find
    * ...to simplify things a bit more, the caller should 
    * allocate memory for the new node, and just pass in
    * the node itself. The caller should provide a "translate_block'
    * function that takes offsets relative to the global buffer
    * and turns them into a thread-local VA to use. 
    */


    static std::size_t size( self_t *ll )
    {
        return( ll->node_count );
    }

   /**
    * insert - insert a node that is allocated at the parameter
    * given offset, then return true if the node was successfully
    * inserted. Once inserted, it is assumed to be the property of
    * the buffer, the callee should not deallocate until "remove"
    * is called.
    * @param data_buffer_base - base of overall buffer address wrt the 
    * callers address space.
    * @param new_ndoe - ipc::ptr_offset_t with offset of allocated
    * and initialized node to add. 
    * @return bool - true assuming the node was added successfully.
    */
    static bool insert( void *data_buffer_base,
                        const ipc::ptr_offset_t new_node,
                        self_t *ll )
    {

        switch( new_node )
        {
            case( ipc::invalid_ptr_offset ):
            case( ipc::ptr_not_found ):
            {
                //intentional fall through
                return( false );
            }
            break;
            default:
            {
                if( ll->node_count == 0 )
                {
                    ll->start = ll->end = new_node;
                }
                else /** there's something in the list **/
                {
                    //usual case here
                    auto *start_node = (NODE_TYPE*)
                        TRANSLATE::translate_block(   data_buffer_base,
                                                ll->end );
                    auto *new_node_ptr = (NODE_TYPE*)
                        TRANSLATE::translate_block(   data_buffer_base,
                                                new_node );
                    NODE_TYPE::add_link(  start_node, ll->end, new_node_ptr, new_node );
                    /** increment end pointer **/
                    ll->end = new_node;
                }                                           
                ll->node_count++;
                return( true );
            }
        }
    }

   /**
    * remove - remove the node at the given offset. If the node is successfully
    * removed, the offset that was given as the parameter is returned, if it was
    * not successful then the return value is set to ipc::invalid_ptr_offset.
    * @param node_to_remove - a valid offset the user wishes to remove
    * @return - value of offset removed, which is up to the callee to actually
    * deallocate, once returned, however, it is removed from the linked list
    * that was created.
    */
    static ipc::ptr_offset_t remove( void *data_buffer_base, 
                                     const ipc::ptr_offset_t node_to_remove,
                                     self_t *ll )
    {
        /** find node VA relative to our space **/
        auto *curr_node = (NODE_TYPE*)
            TRANSLATE::translate_block(   data_buffer_base,
                                    node_to_remove );
        NODE_TYPE *previous = (
            curr_node->prev == ipc::nodebase::init_offset() ? nullptr : 
            (NODE_TYPE*) TRANSLATE::translate_block( data_buffer_base,
                                               curr_node->prev ));
        
        NODE_TYPE *next = (
            curr_node->next == ipc::nodebase::init_offset() ? nullptr :
            (NODE_TYPE*) TRANSLATE::translate_block( data_buffer_base,
                                               curr_node->next ));
        auto remove_head = [&]()
        {
            /** next is the one we want to make the start..momentarily **/
            ll->start = curr_node->next;
            next->prev = ipc::nodebase::init_offset();
            next->next = ipc::nodebase::init_offset();
            curr_node->prev = ipc::nodebase::init_offset();
            curr_node->next = ipc::nodebase::init_offset();
            ll->node_count--;
        };

        auto remove_tail = [&]()
        {
            /** next is the one we want to make the start..momentarily **/
            ll->end = curr_node->prev;
            
            previous->prev = ipc::nodebase::init_offset();
            previous->next = ipc::nodebase::init_offset();

            curr_node->prev = ipc::nodebase::init_offset();
            curr_node->next = ipc::nodebase::init_offset();

            ll->node_count--;
        };

        auto remove_middle = [&]()
        {
                assert( previous  != nullptr );
                assert( curr_node != nullptr );
                assert( next      != nullptr );

                previous->next = curr_node->next.load( std::memory_order_relaxed );
                next->prev = curr_node->prev;
                curr_node->prev = ipc::nodebase::init_offset();
                curr_node->next = ipc::nodebase::init_offset();
                ll->node_count--;
        };

        /** remove node **/
        switch( ll->node_count )
        {
            case( 0 ):
            {
                return( ipc::ptr_not_found );
            }
            break;
            case( 1 ):
            {
                /** check offset to see if it matches, if so return **/
                if( ll->start == node_to_remove && ll->end == node_to_remove )
                {
                    curr_node->next = ipc::nodebase::init_offset(); 
                    curr_node->prev = ipc::nodebase::init_offset();
                    //remove
                    ll->start = ll->end = 0;
                    ll->node_count--;
                    return( node_to_remove );
                }
            }
            break;
            case( 2 ):
            {
                /**
                 * are we removing the head or tail 
                 */
                if( ll->start == node_to_remove )
                {
                    assert( ll->end == curr_node->next );
                    remove_head();
                    return( node_to_remove );
                }
                else if( ll->end == node_to_remove )
                {
                    assert( ll->start == curr_node->prev );
                    remove_tail();
                    return( node_to_remove );
                }
            }
            break;
            default: //greater than 2
            {
                if( node_to_remove == ll->start )
                {
                    remove_head();
                }
                else if( node_to_remove == ll->end )
                {
                    remove_tail();
                }
                else 
                {
                    remove_middle();
                }
                //fix up old pointers
                return( node_to_remove );
            }
        }
        /**
         * we're in some invalid state, return not found, 
         * figure out in the caller.
         */
        return( ipc::ptr_not_found );
    }

   /**
    * find - find the node that is within the linked list. If the node that
    * is given is found within the linked list (using the compare function)
    * then the offset of that node is returned, otherwise the return value
    * that is given is ipc::ptr_not_found is returned.
    * @param node_to_find - node of type NODE_TYPE that is wanted, notice
    * that it is passed by reference and is constant, meaning the user
    * must construct a dummy node to find.
    * @return ipc::ptr_offset_t - a valid pointer offset within the linked
    * list if it exists, else ipc::ptr_not_found is returned. 
    */
    static ipc::ptr_offset_t find(  void *data_buffer_base,
                                    const NODE_TYPE &node_to_find, 
                                    self_t *ll )
    {
        ipc::ptr_offset_t output     = ipc::ptr_not_found;                                       
        /**
         * empty, no start
         */
        if( ll->node_count == 0 )
        {
            return( output );    
        }
        auto curr_ind   = ll->start;
        while( true )
        {
            auto *curr_node = (NODE_TYPE*)
                TRANSLATE::translate_block(   data_buffer_base,
                                        curr_ind );
            if( *curr_node == node_to_find )
            {
                output = curr_ind;
                break;
            }
            const auto the_next = curr_node->next.load( std::memory_order_relaxed ) ;
            /**
             * termination condition, we'll hit this eventually
             */
            if( the_next == ipc::nodebase::init_offset() )
            {
                output = ipc::ptr_not_found;
                break;
            }
            curr_ind = curr_node->next;
        }
        return( output );
    }
    

private:
    std::int64_t      node_count        = 0;
    ipc::ptr_offset_t start             = ipc::invalid_ptr_offset;
    ipc::ptr_offset_t end               = ipc::invalid_ptr_offset;
};
} /** end namespace ipc **/
#endif /* END LOCK_LL_HPP */
