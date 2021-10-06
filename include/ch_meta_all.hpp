/**
 * ch_meta_all.hpp - 
 * @author: Jonathan Beard
 * @version: Mon Aug  2 13:08:46 2021
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
#ifndef CH_META_ALL_HPP
#define CH_META_ALL_HPP  1
#include "bufferdefs.hpp"
#include <cstdint>
#include "sem.hpp"

namespace ipc
{
    
struct alignas( L1D_CACHE_LINE_SIZE ) ch_meta_all
{
    
    constexpr ch_meta_all() = default;
    constexpr ch_meta_all( const ipc::channel_id_t ch_id ) : channel_id( ch_id ){}

    /** this thread id **/
    ipc::channel_id_t   channel_id                        = 0;
   
    ipc::refcnt_t       ref_count_prod                    = 0; 
    ipc::refcnt_t       ref_count_cons                    = 0; 
    ipc::refcnt_t       ref_count_shd                     = 0; 

    ipc::channel_type   type                              = ipc::spsc_record;
    ipc::sem::sem_key_t channel_semaphore                 = { 0 };
    /**
     * FIXME - consider making these a union or template dep.
     * vs. extra space, for right now we're eating up an entire 
     * "block" anyways so who cares, but, definitely a place
     * that'd be nice to optimize.
     */
    alignas( L1D_CACHE_LINE_SIZE ) ptr_offset_t       dummy_node_offset   = ipc::invalid_ptr_offset;
    alignas( L1D_CACHE_LINE_SIZE ) ipc::credit_t      prod_credits        = 0;
    alignas( L1D_CACHE_LINE_SIZE ) ipc::credit_t      cons_credits        = 0;
};

} /** end namespace ipc **/
#endif /* END CH_META_ALL_HPP */
