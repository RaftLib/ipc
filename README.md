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
you want till you run out of memory. 

## Simple example
```cpp
/** only include file you should need **/
#include "buffer.hpp"

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
/** get some memory and do something with it **/
int *output = (int*)
ipc::buffer::allocate_record( tls_producer, 
                              sizeof( int ), 
                              channel_id );

/** send the record **/
while( ipc::buffer::send_record( tls_producer, 
                                 channel_id, 
                                 (void**)&output ) 
                                 != ipc::tx_success );

//NOTE: consumer side has equiv receive record

/** close tls **/
ipc::buffer::close_tls_structure( tls_producer );

/** destroy the buffer, call unlink if you're done with it **/
ipc::buffer::destruct( buffer, "thehandle", true );
```

# Compiling

Currently only for Linux systems. Will add OS X and Windows 
(likely OS X first) soon. Compiles with gcc 9+ and clang 10+. 

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


