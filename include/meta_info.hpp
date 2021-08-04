/**
 * meta_info.hpp - 
 * @author: Jonathan Beard
 * @version: Sun Jul 11 05:51:32 2021
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
#ifndef METAINFO_HPP
#define METAINFO_HPP  1
#include "buffer_debug.hpp"
#include <cstddef>
#include <cstdint>
#include "bufferdefs.hpp"
#include "allocheap.hpp"
#include "translate.hpp"
#include "lock_ll.hpp"
#include "mpmc_lock_free.hpp"
#include "spsc_lock_free.hpp"
#include "channelindex.hpp"
#include "recordindex.hpp"


namespace ipc
{

class meta_info : 
    public translate_helper
#ifdef DEBUG
    ,public buffer_debug
#endif
{
private:
    using sem_buffer_t = char[ semaphore_length ];

public:
    
    using heap_t            = alloc::heap< buffer_size_pow_two, block_size_power_two >;
    using channel_list_t    = lock_ll< channel_index_t, translate_helper >;
    using mpmc_lock_free    = ipc::mpmc_lock_free_queue< ipc::channel_info,
                                                         ipc::record_index_t, 
                                                         translate_helper >;
    
    using spsc_lock_free    = ipc::spsc_lock_free_queue< ipc::channel_info,
                                                         void,
                                                         translate_helper >;
    meta_info()     = default;
    ~meta_info()    = default;

    heap_t                        heap;
    channel_list_t                channel_list;

    sem_buffer_t                  index_sem_name    = { '\0' };
    sem_buffer_t                  alloc_sem_name    = { '\0' };


    /**
     * allocated_size - currently allocated entire buffer
     * size, including all metadata. 
     */
    std::size_t             allocated_size          = 0;

    /** 
     * buffer_size - databuffer size by itself. 
     */
    std::size_t             databuffer_size         = 0;
    
    /**
     * ordering here matters, this will be
     * an in-place allocation for the caller
     * so, these should be laid out in order
     * within the target data region. Only 
     * one threaed can set the cookie. 
     */
    std::uint16_t    cookie                         = 0;
};

} //end namespace ipc

#endif /* END METAINFO_HPP */
