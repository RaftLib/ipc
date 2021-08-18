/**
 * sem.hpp - 
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
#ifndef SEM_HPP
#define SEM_HPP  1

#include <unistd.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <semaphore.h>
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <vector>


#if (_SEM_SEMUN_UNDEFINED == 1) && (_USE_SYSTEMV_SEM_ == 1)
extern "C"
{
union semun {
    int              val;
    struct semid_ds *buf;
    unsigned short  *array;
#ifdef __linux
    struct seminfo  *__buf;
#endif
};

} /** end extern C **/
#endif

namespace ipc
{
struct sem
{
static const std::int32_t sem_read_write
    = 
#if __linux
    (O_RDWR)
#elif __APPLE__ 
#ifndef SEM_R
//assume neither are defined
    0666
#else
    (SEM_R | SEM_A) 
#endif    
#endif
;

static const std::int32_t sem_create
    =
#if __linux && (_USE_POSIX_SEM_ == 1)
    (O_CREAT | O_EXCL)
#elif _USE_SYSTEMV_SEM_
    (IPC_EXCL | IPC_CREAT) 
#endif
;


static const std::int32_t file_rdwr
    =
    (S_IWUSR | S_IRUSR)
;

using sem_key_t
    =
#if (_USE_POSIX_SEM_ == 1) 
        char*
#elif (_USE_SYSTEMV_SEM_ == 1)
        key_t
#endif
;

using sem_obj_t = 
#if (_USE_POSIX_SEM_ == 1)
    sem_t*
#elif (_USE_SYSTEMV_SEM_ == 1)
    /** most likelyi std::int32_t on most systems, but just in case **/
    int
#endif
;

static constexpr sem_obj_t sem_error
    =
#if _USE_POSIX_SEM_ == 1
    SEM_FAILED
#elif _USE_SYSTEMV_SEM_ == 1
    -1 
#endif
;

static constexpr std::int32_t uni_error = -1;

static constexpr sem_obj_t sem_init_value
    =
#if _USE_POSIX_SEM_ == 1
    nullptr
#elif _USE_SYSTEMV_SEM_ == 1
    -1 
#endif
;


/**
 * generate_key - make a key that matches the type needed
 * by either SystemV (OS X) or POSIX semaphores.
 * @return - key, type of sem_key_t (note, type varies by
 * platform.
 */
static sem_key_t generate_key( const int max_length );

template < class T > static sem_key_t convert_key( T k )
{
    return( *reinterpret_cast< ipc::sem::sem_key_t* >( &k ) );
}

static void free_key( sem_key_t k );

/**
 * key_copy - call to copy the key you've created using
 * generate_key above. This is primarily used when you 
 * want to save the key within a shared memory segment
 * or file that is accessible to all. 
 * @param dst - allocated destination buffer
 * @param dst_length - size in bytes of dst buffer
 * @param key - key to copy, depending on the type
 * it could be a string, or some sized integer. 
 * @return true of dst was large enough to provide
 * a copy, false otherwise.
 */
static bool      key_copy( void *dst, 
                           const std::size_t dst_length, 
                           const sem_key_t   key );

/**
 * open - open a semaphore. key must be provided 
 * using "generate_key". Flags and fdperms must
 * use the ipc::sem permissions, not those provided
 * by underlying libraries. 
 * @param key - key from generate_key,
 * @param flags - flags from the ipc::sem namespace
 * @param fdperms - permissions from the ipc::sem namespace
 * @return - valid semaphore, either a pointer or integer
 * depending on the underlying type. 
 */
static sem_obj_t open(  const sem_key_t key, 
                        const std::int32_t flags, 
                        const std::int32_t fdperms );

/**
 * main_init - used by the main process or thread 
 * within the application, this should be called
 * first. 
 * @param id - valid sem_obj_t provided by "open"
 * function
 * @return, (-1) returned on failure, otherwise
 * nonzero.
 */
static int main_init( ipc::sem::sem_obj_t id );

/**
 * sub_init - called by child/subordinate threads. 
 * @param id - valid sem_obj_t
 * @return - (-1) if failure, (0) otherwise
 */
static int sub_init( const ipc::sem::sem_obj_t id );

/**
 * close - to be called when calling process is
 * done with the current semaphore object, frees
 * resources associated with this process. This, 
 * however, does not release the semaphore. 
 * @param obj - valid sem_obj_t.
 * @return - (-1) if failure, (0) otherwise
 */
static int close( ipc::sem::sem_obj_t obj );

/**
 * final_close - called by main thread/process to
 * destroy and free any system resources associated
 * with the semaphore. This should not be called while
 * others are using the semaphore, UB results. 
 * @param obj - valid sem_obj_t
 * @return - (-1) if failure, (0) otherwise
 */
static int final_close( ipc::sem::sem_key_t key );

/**
 * wait - wait on the given semaphore
 * @param key - valid sem_obj_t
 * @return - (-1) if failure, (0) otherwise
 */
static int wait( ipc::sem::sem_obj_t key ); 

/**
 * post - post on the given semaphore
 * @param key - valid sem_obj_t
 * @return - (-1) if failure, (0) otherwise
 */
static int post( ipc::sem::sem_obj_t key );

};

}

#endif /* END SEM_HPP */
