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
#include "genericnode.hpp"
#include "translate.hpp"
#include "lock_ll.hpp"


int main()
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
    
    
    if( offset_1 != ll::find( buffer, (*node_1), &local_ll ) )
    {
        std::cout << "expected (1), found (" << offset_1 << ")\n"; 
        free( buffer );
        return( EXIT_FAILURE );
    }
    free( buffer );
    std::cout << "success\n";
    return( EXIT_SUCCESS );
}
