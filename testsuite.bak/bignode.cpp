/**
 * bignode.hpp - 
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
#include <cstring>

struct empty
{
    char dummy[ 65 ];
};

int main()
{
    ipc::node< empty /** type doesn't matter here **/>  n( ipc::nodebase::sentinel );
    if( sizeof( n ) == 128 /** should always return a multiple of the cache line **/ )
    {
        return( EXIT_SUCCESS );
    }
    return( EXIT_SUCCESS );
}
