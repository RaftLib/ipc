/**
 * shared_seg.hpp - this header doesn't do too much at the moment, 
 * but wanted to make sure there is a sep. place to do shared mem
 * initialization outside of just throwing it in one large cpp file
 * that is buffer.cpp. 
 *
 * @author: Jonathan Beard
 * @version: Wed Oct  6 05:37:14 2021
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
#ifndef SHARED_SEG_HPP
#define SHARED_SEG_HPP  1
#include <functional>
#include "ch_meta_all.hpp"


namespace ipc
{
template < class PARENTNODE, class TRANSLATE > class shared_seg
{
public:
    using init_func_t = std::function< void( void* ) >;

    shared_seg()  = delete;
    ~shared_seg() = delete;
    /**
     * init - initialize shared segment, provide init func
     * as optional for users to initialize the segment to 
     * specific values before it's handed back to anyone.
     */
    static void init( void *buffer_base, 
                      void *channel_base,
                      void *seg_base, 
                      init_func_t f = nullptr )
    {
        /** looks a bit odd given we're going to reuse the channel structure **/ 
        auto *node = (PARENTNODE*)channel_base;
        //get offset to add
        const auto offset_to_add = 
                TRANSLATE::calculate_block_offset( buffer_base,
                                                   seg_base );
        /** 
         * set queue head entry to shared mem seg. Ultimately
         * the reason we re-used the shm segment is we wanted
         * to give the programmer a windowed or named shared mem
         * region without having to set up sep. channels so, these
         * can be "sub-channels" at some future point. 
         */
        node->spsc_q.entry[0] = offset_to_add;
        if( f != nullptr ){ f( seg_base ); }
        return;
    }

    static ipc::tx_code get_segment( PARENTNODE *channel, 
                                     void **receive_node, 
                                     void *buffer_base )
    {
        /** since we're reusing the spsc control structure, return head **/
        const auto offset = 
            channel->spsc_q.entry[0];
        /** should always succeed **/
        *receive_node = TRANSLATE::translate_block( buffer_base, 
                                                    offset );
        return( ipc::tx_success );
    }
};

} /** end namespace ipc **/
#endif /* END SHARED_SEG_HPP */
