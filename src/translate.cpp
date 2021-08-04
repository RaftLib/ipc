/**
 * translate.cpp - 
 * @author: Jonathan Beard
 * @version: Thu Jul  8 07:18:22 2021
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
#include "translate.hpp"
#include "bufferdefs.hpp"
#include <cassert>

void*
ipc::translate_helper::translate( void        *base,
                                  const ipc::ptr_t  offset )
{
    if( offset == 0 ) 
    {
        return( base );
    }
    /** some simple sanity checks **/
    const ipc::ptr_t safe_offset_start = 0;
    const ipc::ptr_t safe_offset_end   = (1 << ipc::buffer_size_pow_two);

    if( offset < safe_offset_start || offset > safe_offset_end )
    {
        return( nullptr );
    }
    //else
    const auto base_address = (std::uintptr_t)base;
    return( (void*)(offset + base_address) );
}
    

void* 
ipc::translate_helper::translate_block( void *base, const ipc::ptr_t block_offset )
{
    return( ipc::translate_helper::translate( base, block_offset * (1 << ipc::block_size_power_two ) ) );
}

    

ipc::ptr_t
ipc::translate_helper::calculate_buffer_offset(  const void * const buffer_base, 
                                            const void * const address )
{
    if( address == nullptr )
    {
        return( ipc::invalid_ptr_offset );
    }

    //sanity check, let's see where the data buffer should be
    const ipc::ptr_t safe_offset_start = 0;
    const ipc::ptr_t safe_offset_end   = (1 << ipc::buffer_size_pow_two);
    
    /** calculate offset **/
    const auto int_ptr_address = (std::uintptr_t)(address);
    const auto int_ptr_head    = (std::uintptr_t)(buffer_base);
    const auto offset = (int_ptr_address - int_ptr_head);

    /**
     * Test for underflow
     */
    if (int_ptr_address < int_ptr_head )
    {
        return( ipc::invalid_ptr_offset );
    }
    /**
     * offset should be past the data array or
     * equal to it.
     */
    if( offset < safe_offset_start )
    {
        return( ipc::invalid_ptr_offset );
    }
    if( offset > safe_offset_end )
    {
        return( ipc::invalid_ptr_offset );
    }

    /** return a good offset **/
    return( offset );
}
    

ipc::ptr_offset_t 
ipc::translate_helper::calculate_block_offset( const void * const buffer_base,
                                               const void * const address )
{
    const auto address_int = (std::uintptr_t) address;
    const auto buffer_int  = (std::uintptr_t) buffer_base;
    assert( address_int >= buffer_int );
    const auto diff        = address_int - buffer_int;
    return( (ipc::ptr_offset_t) ( diff / (1 << ipc::block_size_power_two ) ) );
}
