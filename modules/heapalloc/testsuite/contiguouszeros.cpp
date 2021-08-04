/**
 * contiguouszeros.cpp - 
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
    
    heap_t theheap; 
    heap_t::initialize( &theheap );
    
    /** set the last two bits, ensure we have the right width on the integers to set **/
    alloc::_512bits::set_bit( 511, &theheap.offsetArray[ 0 ] );
    alloc::_512bits::set_bit( 510, &theheap.offsetArray[ 0 ] );
    auto ret_val = alloc::_512bits::find_longest_contiguous_zeros( &theheap.offsetArray[ 0 ] );
    if( ret_val.first != 510 && ret_val.second != 2 )
    {
        return( EXIT_FAILURE );
    }

    //test some known patterns
    auto *harr_1 = &theheap.offsetArray[ 1 ];
    for( auto i( 0 ); i <= 399; i++ )
    {
        alloc::_512bits::set_bit( i, harr_1 );
    }
    auto ret_val2 = alloc::_512bits::find_longest_contiguous_zeros( harr_1 );
    if( ret_val2.first != 112 && ret_val2.second != 400 )
    {
        return( EXIT_FAILURE );
    }
    
    //unset pattern
    for( auto i( 0 ); i <= 512; i++ )
    {
        alloc::_512bits::unset_bit( i, harr_1 );
    }
    //set new pattern
    for( auto i( 0 ); i <= 255; i++ )
    {
        alloc::_512bits::set_bit( i, harr_1 );
    }
    auto ret_val3 = alloc::_512bits::find_longest_contiguous_zeros( harr_1 );
    if( ret_val3.first != 256 && ret_val3.second != 256 )
    {
        return( EXIT_FAILURE );
    }
    
    
    //unset pattern
    for( auto i( 0 ); i <= 512; i++ )
    {
        alloc::_512bits::unset_bit( i, harr_1 );
    }
    ret_val3 = alloc::_512bits::find_longest_contiguous_zeros( harr_1 );
    if( ret_val3.first != 512 && ret_val3.second != 0 )
    {
        return( EXIT_FAILURE );
    }

    std::cout << "SUCCESS\n";
    return( EXIT_SUCCESS );
}
