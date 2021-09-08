/**
 * bufferdefs.hpp - 
 * @author: Jonathan Beard
 * @version: Thu Apr  9 05:27:43 2020
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
#ifndef _BUFFERDEFS_HPP_
#define _BUFFERDEFS_HPP_  1
#include <cstdint>
#include <limits>
#include <atomic>
#include <map>
#include <memory>
#include <type_traits>
#include <array>
#include <string>

#include <sys/types.h>

#include "ipc_moduleflags.hpp"

#ifndef UNUSED 
#ifdef __clang__
#define UNUSED( x ) (void)(x)
#else
#define UNUSED( x )[&x]{}()
#endif
//FIXME need to double check to see IF THIS WORKS ON MSVC
#endif

namespace ipc
{
    /**
     * for now we use static buffer and static sizing
     */
    static constexpr std::uint8_t block_size_power_two = 12; //4KiB
    static constexpr std::uint8_t buffer_size_pow_two  = 30; //1GiB
    
    /** 
     * ptr_offset_t - to represent offsets within 
     * linked lists and like things inside our 
     * buffer. Keep in mind these should 
     * never be VA addresses but relative offsets
     * from the base of the struct.
     */
    using ptr_offset_t  = std::int64_t;



    enum ptr_err_t : ptr_offset_t
    {
        invalid_ptr_offset = std::numeric_limits< ptr_offset_t >::min(),
        ptr_not_found      = std::numeric_limits< ptr_offset_t >::min() + 1,
        valid_offset       = 0
    };
    
    
    using refcnt_t       = std::int32_t;

    using channel_id_t   = ptr_offset_t;
    
    enum channel_err_t : channel_id_t
    {
        null_channel            = std::numeric_limits< channel_id_t >::min() + 2,
        channel_not_found       = std::numeric_limits< channel_id_t >::min() + 3,
        channel_exists          = std::numeric_limits< channel_id_t >::min() + 4,
        channel_added           = std::numeric_limits< channel_id_t >::min() + 5,
        channel_err             = std::numeric_limits< channel_id_t >::min() + 6,
        channel_alloc_err       = std::numeric_limits< channel_id_t >::min() + 7
    };

    
    enum channel_type : int
    {
        spsc_record,
        mpmc_record,
        atomic,
        spsc_data,
        mpmc_data,
        number_channel_types

    };

    constexpr static std::array< char[30], ipc::number_channel_types > 
        channel_type_names = 
       {{
             "spsc_record",
             "mpmc_record",
             "atomic",
             "spsc_data",
             "mpmc_data",
        }};

    using credit_t = std::atomic< std::uint32_t >;
    using ctrl_ptroffset_t = std::atomic< ipc::ptr_offset_t >;

    using wrap_t        = std::atomic< std::uint64_t >;
    /** 
     * might move this one to a separate file, but
     * this one describes the thread id type within
     * the threadinfo.hpp file. 
     */
    using thread_id_t = std::int32_t;
    
    /**
     * type to be used for offset pointer
     * calculations.
     */
    using ptr_t     =   ipc::ptr_offset_t;
    

    /**
     * sanity check for portability.
     */
    static_assert( sizeof( pid_t ) < sizeof( std::uint64_t ), 
                   "64b unsigned is to small to represent thread size" );
    
    
    /**
     * type for buffer labeling, primarily for debugging
     */
    using node_id_t = std::uint64_t;
    

    /**
     * type for data, given not all systems
     * have 8-bits per byte, this should 
     * standardize that a bit (although
     * almost all modern systems do use 8b/B
     */
    using byte_t    =   std::uint8_t;


    /**
     * tx_code - return types for the send/receive
     * operations on each channel. 
     */
    enum tx_code : std::int32_t
    {
        /** add specific codes from min/max throug (-1) **/
        no_such_channel = std::numeric_limits< std::int32_t >::min(),
        tx_dummy   = -4,
        tx_empty   = -3,
        tx_retry   = -2,        
        /** standard error codes, start at zero for succ and go neg **/
        tx_error   = -1,
        tx_success = 0
    };

    enum direction_t : std::int8_t
    {
        dir_not_set,
        producer,
        consumer
    };

    using channel_map_t = std::shared_ptr< std::map< channel_id_t, channel_type > >;
    inline static auto make_channel_map(){ 
        return( std::make_shared< std::map< channel_id_t, channel_type > >() ); 
    }
} /** end namespace ipc **/

#endif /* END _BUFFERDEFS_HPP_ */
