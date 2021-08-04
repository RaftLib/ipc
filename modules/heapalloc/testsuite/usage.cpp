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
    

using heap_t = alloc::heap< 30, 12 >;

struct foo
{
    foo() 
    {
        heap_t::initialize( &theheap );
    }
    heap_t theheap; 
};

int main()
{
    auto *t = new foo();
    for( auto i( 0 ); i < 10; i++ )
    { 
        std::cout << heap_t::get_n_blocks( 512, &t->theheap ) << "\n";
    }
    delete( t );
    return( EXIT_SUCCESS );
}
