/**
 * buffer_base.hpp - 
 * @author: Jonathan Beard
 * @version: Sat Jul 10 06:35:11 2021
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
#ifndef BUFFER_BASE_HPP
#define BUFFER_BASE_HPP  1

#include <cstdint>
#include <cstddef>
#include "padsize.hpp"
#include "meta_info.hpp"


namespace ipc
{



class buffer_base :
 public meta_info
{
protected:
    /**
     * when you in-place allocate this class,
     * this will take up all the rest of the space within the 
     * SHM region. It'll start on the first....
     */
    static constexpr std::uint16_t cookie_in_use     = 0x1337;

private:

    constexpr static auto pad_size = 
        ipc::findpad< ( 1 << block_size_power_two ) /** block size **/,
                      ipc::meta_info
                      >::calc();
public:
    buffer_base();

    ~buffer_base()  = default;

    
    /**
     * NOTE: you should add anythying with in-buffer
     * storage requirements to the "meta_info" class
     * so that the padding is calculated correctly.
     */

    /**
     * padding, scribble with all ones at init
     */
    ipc::byte_t      alignment_padding[ pad_size ] = 
        { std::numeric_limits< ipc::byte_t >::max() };

}; //end struct buffer_base

} //end namespace ipc

#endif /* END BUFFER_BASE_HPP */
