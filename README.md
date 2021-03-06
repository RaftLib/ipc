# ABOUT

This is a relatively simple IPC buffer that allows multiple processes
and threads to share a dynamic heap allocator, designate "channels"
between processes, and share that memory between producer/consumer 
pairs on those channels. 

Currently implemented is the following:
- multi-threaded/multi-process allocation via priority heap
- per-thread TLS lock-free allocation slab
- single-producer, single-consumer channel (multi-process and multi-threaded)
- multiple channels per buffer (you can have as many independent channels as
you want till you run out of memory).
- named static shared memory segment (as a channel) which can be used for synchronization or other purposes where message passing semantics aren't useful. 

## Status - [![CI](https://github.com/RaftLib/ipc/actions/workflows/main.yml/badge.svg)](https://github.com/RaftLib/ipc/actions/workflows/main.yml)
Currently testing the following combinations
- Linux
  - POSIX SHM + POSIX Semaphores
  - POSIX SHM + SystemV Semaphors
  - SystemV SHM + POSIX Semaphores
  - SystemV SHM + SystemV Semaphors
- OS X
  - POSIX SHM + SystemV Semaphores
  



## Why
* Lack of existing simple point-to-point multi-process
allocation and communication mechanisms. 

## Simple example
```cpp
/** only include file you should need **/
#include <buffer>

/**
 * shared memory buffer allocated and returned as a pointer,
 * this function can be called from each process safely to get
 * the file descriptor opened and memory opened within your 
 * address space. 
 */
auto *buffer = ipc::buffer::initialize( "thehandle"  );

/** get a thread local data structure for the buffer with thread id **/
auto *tls_producer      = ipc::buffer::get_tls_structure( buffer,
                                                          thread_id );
const auto channel_id = 1;
/**
 * allocate a channel that is a single-producer, single consumer
 * record channel, record channels are used for larger allocations
 * up to 1MiB. The consumer must know that channel_id is the correct
 * channel and also the type of channel otherwise an error will be 
 * thrown. 
 */
ipc::buffer::add_spsc_lf_record_channel( tls_producer, channel_id )

/** get some memory and do something with it **/
int *output = (int*)
/** currently single allocations limited to 1MiB **/
ipc::buffer::allocate_record( tls_producer, 
                              sizeof( int ), 
                              channel_id );

/** lock-free record send **/
while( ipc::buffer::send_record( tls_producer, 
                                 channel_id, 
                                 (void**)&output ) 
                                 != ipc::tx_success );

//NOTE: consumer side has equiv lock-free receive record

/** close tls **/
ipc::buffer::close_tls_structure( tls_producer );

/** destroy the buffer **/
ipc::buffer::destruct( buffer       /** buffer ptr  **/, 
                       "thehandle"  /** file handle **/, 
                       true         /** unlink, true if you're the last user **/);
```

# Compiling

Currently only for Linux and OS X systems. Will add Windows 
soon. Compiles with gcc 9+ and clang 10+. 

```
git clone https://github.com/RaftLib/ipc.git
cd ipc
cmake ../ 
make -j
make test
```

# Usage notes
Will add more notes soon. Most complete example with two 
processes and two threads (1 process communicating with 
another process that has two threads) is [here](https://github.com/RaftLib/ipc/blob/main/testsuite/spsc_two_processes_multi_channel.cpp)

- NOTE: the combination of OS X + SystemV Shared Memory + SystemV Semaphores is currently 
not passing tests, will investigate. All other combinations on OS X and Linux work. 
