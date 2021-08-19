/**
 * sem.cpp - 
 * @author: Jonathan Beard
 * @version: Thu Aug 12 15:06:25 2021
 * 
 * Copyright 2021 Jonathan Beard
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
#include <string>
#include <random>
#include <sys/errno.h>
#include <cstring>
#include <cassert>
#include <limits>
#include "sem.hpp"

#ifndef UNUSED 
#ifdef __clang__
#define UNUSED( x ) (void)(x)
#else
#define UNUSED( x )[&x]{}()
#endif
//FIXME need to double check to see IF THIS WORKS ON MSVC
#endif

ipc::sem::sem_key_t 
ipc::sem::generate_key( const int max_length, const int proj_id )
{
#if _USE_POSIX_SEM_ == 1
    //string key
    std::random_device rd;
    std::mt19937 gen( rd() );
    std::uniform_int_distribution<> distrib( 0, std::numeric_limits< int >::max() );
    const int val = distrib( gen );
    return( strdup( std::to_string( val ).substr( 0, max_length ).c_str() ) );
#elif _USE_SYSTEMV_SEM_ == 1
    UNUSED( max_length );
    //integer key
    char *path = getcwd( nullptr, 0 );
    if( path == nullptr )
    {
        std::perror( "failed to get cwd" );
        exit( EXIT_FAILURE );
    }
    const auto output = ftok( path, proj_id);
    free( path );
    return( output );
#endif
}

void 
ipc::sem::free_key( sem_key_t k )
{
#if _USE_POSIX_SEM_ == 1
    free( k );
#else
    UNUSED( k );
#endif
}

bool      
ipc::sem::key_copy(         ipc::sem::sem_key_t &dst,
                    const   std::size_t         dst_length, 
                    const   ipc::sem::sem_key_t key )
{
#if _USE_POSIX_SEM_ == 1
    //string key
    std::memset( dst, '\0', dst_length );
    std::strncpy(   (char*) dst,
                    key,
                    dst_length );
    return( true );                    
#elif _USE_SYSTEMV_SEM_ == 1
    UNUSED( dst_length );
    //key_t key
    dst = key;
    return( true );
#else
    //not implemented
    return( false );
#endif
}


ipc::sem::sem_obj_t 
ipc::sem::open(     const sem_key_t key, 
                    const std::int32_t flags, 
                    const std::int32_t fdperms )
{
#if _USE_POSIX_SEM_ == 1
    return( sem_open( 
            key, 
            flags, 
            fdperms, 
            0 ) );
#elif _USE_SYSTEMV_SEM_ == 1
    return( semget( key, 1, (flags | fdperms) ) ); 
#endif
}

//should be called before launching children
int 
ipc::sem::main_init( ipc::sem::sem_obj_t id )
{
#if _USE_POSIX_SEM_ == 1
    return( sem_init( id, 1 /** multiprocess **/, 1 ) );
#elif _USE_SYSTEMV_SEM_ == 1
    //should return something other than (-1) on success
    union semun arg; 
    arg.val = 0;
    if( semctl( id, 0, SETVAL, arg ) == -1 )
    {
        //errno is set, ret -1 and errno can be checked
        return( -1 );
    }
    /**
     * use semctl to set the time on the newly created
     * semaphore so that subordinate users of this sem
     * will know that it is actually initialized.
     */
    struct sembuf sop;
    sop.sem_num = 0;
    sop.sem_op  = 1;
    sop.sem_flg = 0;
    //returns (-1) on err
    return( semop( id, &sop, 1 ) );
#else
    //unimplemented
    return( -1 );
#endif
}

int 
ipc::sem::sub_init( const ipc::sem::sem_obj_t id )
{
#if _USE_POSIX_SEM_ == 1
    //nothing to do here
    UNUSED( id );
    return( 0 );
#elif _USE_SYSTEMV_SEM_ == 1
    struct  semid_ds    ds;
    union   semun       arg; 
    arg.buf = &ds;
    while( true )
    {
        if( semctl( id, 0, IPC_STAT, arg ) == -1 )
        {
            //perror set, return -1
            return( -1 );
        }
        if( ds.sem_otime != 0 /** uninitialized by main **/ )
        {
            //now initialized 
            return( 0 );
        }
    }
#else 
    //unimplemented
    return( -1 );
#endif    
}

int
ipc::sem::close( ipc::sem::sem_obj_t obj )
{
#if _USE_POSIX_SEM_ == 1
    return( sem_close( obj ) );
#elif _USE_SYSTEMV_SEM_ == 1
    UNUSED( obj );
    //nothing to do, they're tracked system wide
    //until we destroy
    return( 0 ); 
#else
    UNUSED( obj );
    //unimplemented
    return( -1 );
#endif
}

int 
ipc::sem::final_close( ipc::sem::sem_key_t key )
{
#if _USE_POSIX_SEM_ == 1
    return( sem_unlink( key ) );
#elif _USE_SYSTEMV_SEM_ == 1
    //get key first from key
    const auto _id = ipc::sem::open( key, 0x0, ipc::sem::file_rdwr );
    union semun arg; 
    return( semctl( _id, 0, IPC_RMID, arg ) );
#else
    //unimpleemnted
    return( -1 );
#endif
}

int 
ipc::sem::wait( ipc::sem::sem_obj_t obj )
{
#if _USE_POSIX_SEM_ == 1
    return( sem_wait( obj ) );
#elif _USE_SYSTEMV_SEM_ == 1
    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_flg = 0;
    sops.sem_op  = -1;
    return( semop( obj, &sops, 1 ) );
#else
    //unimplemented
    return( -1 );
#endif
}

int 
ipc::sem::post( ipc::sem::sem_obj_t key )
{
#if _USE_POSIX_SEM_ == 1
    return( sem_post( key ) );
#elif _USE_SYSTEMV_SEM_ == 1
    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_flg = 0;
    sops.sem_op  = 1;
    return( semop( key, &sops, 1 ) );
#else
    //unimplemented
    return( -1 );
#endif
}
