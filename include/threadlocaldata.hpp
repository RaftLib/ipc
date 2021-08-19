/**
 * threadlocaldata.hpp - 
 * @author: Jonathan Beard
 * @version: Fri May  8 09:16:55 2020
 * 
 * Copyright 2020 Jonathan Beard
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
#ifndef _THREADLOCALDATA_HPP_
#define _THREADLOCALDATA_HPP_  1
#include <semaphore.h>
#include <iostream>
#include <map>
#include "bufferdefs.hpp"
#include "channelinfo.hpp"
#include "sem.hpp"

namespace ipc
{

class  buffer; //forward declare, defined in buffer.hpp
struct channel_info; 
struct allocate_metadata;


struct local_allocation_info
{
    /** 
     * this is your free space that you're pushing into the 
     * FIFO. This region is lock-free. It is with respect
     * to block offset, not buffer offset (e.g., block modulo
     * block_size). So if your buffer is 1GiB, then you have
     * (1 << (30 - block_size)) offsets possible here.
     */
    ipc::ptr_offset_t    local_allocation   = ipc::invalid_ptr_offset;

    /**
     * this is a reference to how much space is available
     * that has already been allocated by this thread. If
     * This value is zero, more space must be allocated. 
     */
    std::size_t     blocks_available  = 0;
};

struct thread_local_data
{
    /**
     * self explanatory, but, this semaphore
     * is to guard allocations and ensure 
     * buffers handed out are unique. 
     */
    ipc::sem::sem_obj_t allocate_semaphore;
    /**
     * index_semaphore - local copy of semaphore
     * that is really only needed when opening
     * and closing a new thread, this is set 
     * when you start-up and closed when you
     * exit, otherwise, don't touch this. 
     */
    ipc::sem::sem_obj_t index_semaphore;


    
    /**
     * calling thread id from the producer side,
     * not the consumer. 
     */
    ipc::thread_id_t thread_id = 0;
    /**
     * this is the thread local address space
     * correct pointer into the buffer. It's 
     * needed for so many things that we just
     * keep it here. 
     */
    ipc::buffer *buffer = nullptr;
    

    std::map< ipc::channel_id_t,
              ipc::channel_info* >  channel_map;
    
    std::map< ipc::channel_id_t, 
              ipc::local_allocation_info > channel_local_allocation;

};


inline std::ostream& operator << ( std::ostream &s, ipc::thread_local_data &d )
{
    s << "{\n";
    s << "thread_id   : " << d.thread_id << "\n";
    s << "per-channel info: \n";
    for( const auto ch_pair : d.channel_map )
    {
        ipc::channel_info *ch_info = d.channel_map[ ch_pair.first ];
        const auto &al_info = d.channel_local_allocation[ ch_pair.first ];
        s << (*ch_info) << "\n";
        s << "\tlocal allocation info:\n";
        s << "\tblocks available: " << al_info.blocks_available << "\n";
        s << "\tallocation start: " << al_info.local_allocation << "\n";
    }
    s << "}\n";
    return( s );
}

} /** end namespace ipc **/
#endif /* END _THREADLOCALDATA_HPP_ */
