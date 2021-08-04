/**
 * translate.hpp - 
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
#ifndef TRANSLATE_HPP
#define TRANSLATE_HPP  1
#include "bufferdefs.hpp"
namespace ipc
{


struct translate_helper
{
    /** 
     * translate - simple functio that takes the base address
     * of your data buffer (specifically the data field from
     * ipc::buffer) and takes the offset that is within the 
     * buffer itself, and then returns a pointer that is 
     * within your address spaces's VA. 
     * @param base - void pointer to your VA base data ele
     * @param offset - offset within the buffer itself.
     * @return - valid pointer to data element within your
     * VA space., returns nullptr if offset is out of range
     */
    static void*  translate(   void       *base, 
                               const ipc::ptr_t offset );


    static void* translate_block( void *base, const ipc::ptr_t block_offset ); 
    
    /**
     * calculate_buffer_offset - calculate the offset from a VA, offset is what is
     * used internally. 
     * @param buffer - buffer to calculate offset from
     * @param address - address to get offset from, must be non-null. 
     * @return ptr_t - offset of address.
     */
    static ipc::ptr_t calculate_buffer_offset( const void * const buffer_base, 
                                          const void * const address );

 
    static ipc::ptr_offset_t calculate_block_offset( const void * const buffer_base,
                                                     const void * const address );

};

} //end namespace ipc
#endif /* END TRANSLATE_HPP */
