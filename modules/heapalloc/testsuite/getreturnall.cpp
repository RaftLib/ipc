/**
 * getreturnall.cpp - 
 * @author: Jonathan Beard
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
#include <cstdint>
#include <iostream>
#include <cstdlib>
#include <cassert>
#include <limits>

#define TESTHEAP 1
#include "allocheap.hpp"

int main()
{
    using heap_t = alloc::heap< 30, 12 >;
    
    if( heap_t::numElements != (1 << (30 - 12 - 9) ) )
    {
        exit( EXIT_FAILURE );
    }
    
    heap_t theheap; 
    heap_t::initialize( &theheap );

    //check to see what happens when it's "full", we have 512 blocks of 512
    for( auto i( 0 ); i < 512; i++ )
    {
        if( heap_t::get_blocks_avail( &theheap ) != 512 )
        {
            return( EXIT_FAILURE );
        }
        else
        {
            heap_t::get_n_blocks( 512, &theheap );
        }
    }
    for( std::uint64_t i( 0 ); i < heap_t::numElements; i++ )
    {
        if( theheap.arr[ i ].contiguous_free != 0 )
        {
            return( EXIT_FAILURE );
        }
    }
    
    //let's return all - this is what we're testing.  
    for( std::uint64_t i( 0 ); i < (heap_t::numElements*512); i+= 512 )
    {
        heap_t::return_n_blocks( i, 512, &theheap );
    }
    for( std::uint64_t i( 0 ); i < heap_t::numElements; i++ )
    {
        if( theheap.arr[ i ].contiguous_free != 512 )
        {
            return( EXIT_FAILURE );
        }
    }


    std::cout << "SUCCESS\n";
    return( EXIT_SUCCESS );
}
