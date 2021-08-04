/**
 * alloc.cpp - 
 * @author: Jonathan Beard
 * @version: Thu Jun 18 08:08:03 2015
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
#include <cstring>
#include <cassert>
#include <string>

int
main( int argc, char **argv )
{
   static const auto key_length( 16 );
   std::string key;
   shm::genkey( key, key_length );
   std::int32_t *ptr( nullptr );
   try
   {
      ptr = reinterpret_cast< std::int32_t* >( shm::init( key, 0x1000 ) );
   }
   catch( bad_shm_alloc ex )
   {
      std::cerr << ex.what() << "\n";
      exit( EXIT_FAILURE );
   }
   for( int i( 0 ); i < 100; i++ )
   {  
      ptr[ i ] = i;
   }
   /** if we get to this point then we assume that the mem is writable **/
   shm::close( key, 
               reinterpret_cast<void**>( &ptr), 
               0x1000,
               true,
               true );
   /** should get here and be done **/
   return( EXIT_SUCCESS );
}
