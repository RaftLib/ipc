/**
 * zerobytes.cpp - 
 * @author: Jonathan Beard
 * @version: Thu Jun 18 08:09:25 2015
 * 
 * Copyright 2015 Jonathan Beard
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
#include <shm>

int
main( int argc, char **argv )
{
    std::string key( "" );
    std::string key2( "" );
    shm::genkey( key, 32 );
    shm::genkey( key2, 32 );

    std::cout << "key 1: " << key << "\n";
    std::cout << "key 2: " << key2 << "\n";
    /** 
     * two successive keys should not be equal 
     */
    if( key.compare( key2 ) == 0 )
    {
       std::cerr << "failed key comparision\n";
       exit( EXIT_FAILURE );
    }
    return( EXIT_SUCCESS );
}
