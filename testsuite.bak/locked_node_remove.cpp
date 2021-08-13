/**
 * genericnode.hpp - 
 * @author: Jonathan Beard
 * @version: Fri Sep 12 10:28:33 2014
 * 
 * Copyright 2014 Jonathan Beard
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
#include <cstdlib>
#include <typeinfo>
#include <iostream>
#include <functional>
#include "genericnode.hpp"
#include "translate.hpp"
#include "lock_ll.hpp"
#include "bufferdefs.hpp"

using node_t = ipc::node< int /** type doesn't matter here **/>;

using ll = ipc::lock_ll<    node_t, 
                            ipc::translate_helper >;


using test_func_t = std::function< bool( void*, 
                                          node_t*,
                                          node_t*,
                                          node_t*,
                                          ipc::ptr_offset_t,
                                          ipc::ptr_offset_t,
                                          ipc::ptr_offset_t,
                                          ll* ) >;

static void test_me( test_func_t t )
{
    using node_t = ipc::node< int /** type doesn't matter here **/>;
    
    void *buffer    = malloc( (1 << 20) );
    char *temp      = (char*)buffer;

     
    
    auto *node_0 = new (temp)                                               node_t( 0,  ipc::nodebase::dummy );
    auto *node_1 = new (temp + ( 1 << ipc::block_size_power_two ) )         node_t ( 1, ipc::nodebase::dummy );
    auto *node_2 = new (temp + ( 1 << (ipc::block_size_power_two + 2) ))    node_t ( 2, ipc::nodebase::dummy );
    
    
    using ll = ipc::lock_ll< node_t, ipc::translate_helper >;
    
    ll local_ll;

    const auto offset_0 = 
        ipc::translate_helper::calculate_block_offset( buffer, node_0 );
    
    const auto offset_1 = 
        ipc::translate_helper::calculate_block_offset( buffer, node_1 );
    
    const auto offset_2 = 
        ipc::translate_helper::calculate_block_offset( buffer, node_2 );

    ll::insert( buffer, offset_0, &local_ll );
    ll::insert( buffer, offset_1, &local_ll );
    ll::insert( buffer, offset_2, &local_ll );
    

    

    //auto print_me = [&]()
    //{
    //    std::cout << *node_0   << "\n";
    //    std::cout << *node_1   << "\n";
    //    std::cout << *node_2   << "\n";
    //};
    //original list
    
    if( ! t( buffer, node_0, node_1, node_2, offset_0, offset_1, offset_2, &local_ll ) )
    {
        free( buffer );
        exit( EXIT_FAILURE );
    }
    else
    {
        free( buffer );
    }
}




int main()
{
    auto check_remove_first = []( void *buffer,
                                  node_t *a,
                                  node_t *b,
                                  node_t *c,
                                  ipc::ptr_offset_t o_a,
                                  ipc::ptr_offset_t o_b,
                                  ipc::ptr_offset_t o_c,
                                  ll     *list ) -> bool
    {
        UNUSED( c );
        UNUSED( o_b );
        UNUSED( o_c );
        ll::remove( buffer, o_a , list );
        if( b->prev != ipc::nodebase::init_offset() )
        {
            return( false );
        }
        if( a->next != ipc::nodebase::init_offset() )
        {
            return( false );
        }
        return( true );
    };
    

    auto check_remove_middle = []( void *buffer,
                                  node_t *a,
                                  node_t *b,
                                  node_t *c,
                                  ipc::ptr_offset_t o_a,
                                  ipc::ptr_offset_t o_b,
                                  ipc::ptr_offset_t o_c,
                                  ll     *list ) -> bool
    {
        ll::remove( buffer, o_b , list );
        if( a->next != o_c )
        {
            return( false );
        }
        if( c->prev != o_a )
        {
            return( false );
        }
        if( b->next != ipc::nodebase::init_offset() )
        {
            return( false );
        }
        if( b->prev != ipc::nodebase::init_offset() )
        {
            return( false );
        }
        return( true );
    };
    
    auto check_remove_last = []( void *buffer,
                                  node_t *a,
                                  node_t *b,
                                  node_t *c,
                                  ipc::ptr_offset_t o_a,
                                  ipc::ptr_offset_t o_b,
                                  ipc::ptr_offset_t o_c,
                                  ll     *list ) -> bool
    {
        UNUSED( a );
        UNUSED( o_a );
        UNUSED( o_b );
        ll::remove( buffer, o_c , list );
        if( b->next != ipc::nodebase::init_offset() )
        {
            return( false );
        }
        if( c->next != ipc::nodebase::init_offset() )
        {
            return( false );
        }
        if( c->prev != ipc::nodebase::init_offset() )
        {
            return( false );
        }

        return( true );
    };
    
    auto check_remove_all = []( void *buffer,
                                  node_t *a,
                                  node_t *b,
                                  node_t *c,
                                  ipc::ptr_offset_t o_a,
                                  ipc::ptr_offset_t o_b,
                                  ipc::ptr_offset_t o_c,
                                  ll     *list ) -> bool
    {
        UNUSED( a );
        UNUSED( b );
        UNUSED( c );
        if( ll::size( list ) != 3 )
        {
            return( false );
        }
        ll::remove( buffer, o_b , list );
        if( ll::size( list ) != 2 )
        {
            return( false );
        }
        ll::remove( buffer, o_c , list );
        if( ll::size( list ) != 1 )
        {
            return( false );
        }
        ll::remove( buffer, o_a , list );
        if( ll::size( list ) != 0 )
        {
            return( false );
        }
        return( true );
    };

    test_me( check_remove_first );    
    test_me( check_remove_middle );    
    test_me( check_remove_last );    
    test_me( check_remove_all );    
    
    std::cout << "success\n";
    return( EXIT_SUCCESS );
}
