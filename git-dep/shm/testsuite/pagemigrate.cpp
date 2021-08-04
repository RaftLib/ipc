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
#include <cassert>
#include <string>

#ifdef __linux
/** for get cpu **/
#if (__GLIBC_MINOR__ < 14) && (__GLIBC__ <= 2)

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#else

#ifndef _BSD_SOURCE
#define _BSD_SOURCE 1
#endif

#ifndef _SVID_SOURCE
#define _SVID_SOURCE 1
#endif

#endif /** end glibc check **/
#include <pthread.h>
#include <sched.h>
#include <sys/sysinfo.h>
#include <numaif.h>
#include <numa.h>
void 
set_affinity( const std::size_t desired_core )
{
   /**
    * pin the current thread 
    */
   cpu_set_t   *cpuset( nullptr );
   size_t cpu_allocate_size( -1 );
#if   (__GLIBC_MINOR__ > 9 ) && (__GLIBC__ == 2 )
   const int8_t   processors_to_allocate( 1 );
   cpuset = CPU_ALLOC( processors_to_allocate );
   assert( cpuset != nullptr );
   cpu_allocate_size = CPU_ALLOC_SIZE( processors_to_allocate );
   CPU_ZERO_S( cpu_allocate_size, cpuset );
#else
   cpu_allocate_size = sizeof( cpu_set_t );
   cpuset = (cpu_set_t*) malloc( cpu_allocate_size );
   assert( cpuset != nullptr );
   CPU_ZERO( cpuset );
#endif
   CPU_SET( desired_core,
            cpuset );
   errno = 0;
   if( sched_setaffinity( 0 /* calling thread */,
                         cpu_allocate_size,
                         cpuset ) != 0 )
   {
      perror( "Failed to set affinity for cycle counter!!" );
      exit( EXIT_FAILURE );
   }
   /** wait till we know we're on the right processor **/
   if( sched_yield() != 0 )
   {
      perror( "Failed to yield to wait for core change!\n" );
   }
   return;
}

struct Data
{
   Data( void **ptr ) : ptr( ptr )
   {
      shm::genkey( key_buff, 30 );
   }
   std::string key_buff;
   void **ptr;
};

void* thr( void *ptr )
{
   Data *data( reinterpret_cast< Data* >( ptr ) );
   const auto max_core( get_nprocs_conf() - 1 );
   set_affinity( max_core );
   try
   {
      *data->ptr = reinterpret_cast< std::int32_t* >( 
        shm::init( data->key_buff, 0x1000 ) 
       );
   }
   catch( bad_shm_alloc ex )
   {
      std::cerr << ex.what() << "\n";
      exit( EXIT_FAILURE );
   }
   std::int32_t *ptr_int( reinterpret_cast< std::int32_t* >( *data->ptr ) );
   for( auto i( 0 ); i < 100; i++ )
   {  
      ptr_int[ i ] = i;
   }
   pthread_exit( nullptr );
}

#endif


int
main( int argc, char **argv )
{
   /** no facilities for this exist on OS X so why even try **/
#if __linux
   if( numa_num_configured_nodes() == 1 )
   {
      std::cerr << "only one numa node, exiting!\n";
      /** no point in continuing, there's only one **/
      return( EXIT_SUCCESS );
   }
   set_affinity( 0 );
   const auto my_numa( numa_node_of_cpu( 0 ) );
   std::cerr << "calling thread numa node: " << my_numa << "\n";
   void *ptr( nullptr );
   pthread_t thread;
   Data d( &ptr );
   /** create thread, move to new NUMA node, allocate mem there **/
   pthread_create( &thread, nullptr, thr, &d );
   /** join **/
   pthread_join( thread, nullptr ); 
   /** get current numa node of pages, checking the first should be sufficient **/
   int status[ 1 ];
   move_pages( 0, 1, &ptr, nullptr,status,0 );
   const auto pages_before( status[ 0 ] );
   std::cerr << "numa node before move: " << pages_before << "\n";
   assert( my_numa != pages_before );
   shm::move_to_tid_numa( 0, ptr, 0x1000 );
   move_pages( 0, 1, &ptr, nullptr,status,0 );
   const auto pages_after( status[ 0 ] );
   std::cerr << "numa node after move: " << pages_after << "\n";
   assert( pages_after == my_numa );
   
   /** if we get to this point then we assume that the mem is writable **/
   shm::close( d.key_buff, 
               reinterpret_cast<void**>(&ptr), 
               0x1000,
               true,
               true );
#endif               
   /** should get here and be done **/
   return( EXIT_SUCCESS );
}
