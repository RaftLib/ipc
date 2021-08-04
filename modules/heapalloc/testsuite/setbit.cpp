/**
 * setbit.cpp - 
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
    
    /** set first two to make sure we haven't hosed something basic **/
    alloc::_512bits::set_bit( 1, &theheap.offsetArray[ 0 ] );
    alloc::_512bits::set_bit( 0, &theheap.offsetArray[ 0 ] );
    if( alloc::_512bits::total_bits_set(  &theheap.offsetArray[ 0 ] ) != 2 )
    {
        exit( EXIT_FAILURE );
    }
   
    /** set the last two bits, ensure we have the right width on the integers to set **/
    alloc::_512bits::set_bit( 511, &theheap.offsetArray[ 0 ] );
    alloc::_512bits::set_bit( 510, &theheap.offsetArray[ 0 ] );
    if( alloc::_512bits::total_bits_set(  &theheap.offsetArray[ 0 ] ) != 4 )
    {
        exit( EXIT_FAILURE );
    }
    
    
    std::cout << "SUCCESS\n";
    return( EXIT_SUCCESS );
}
