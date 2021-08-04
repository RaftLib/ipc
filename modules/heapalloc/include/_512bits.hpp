/**
 * _512bits.hpp - 
 * @author: Jonathan Beard
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
#ifndef _512BITS_HPP
#define _512BITS_HPP  1
#include <cstdint>
#include <algorithm>

namespace alloc
{

struct _512bits
{
    static std::uint16_t total_bits_set( _512bits *f );

    static std::pair< std::int32_t, std::int32_t > find_longest_contiguous_zeros( _512bits *f );

    static void set_bit( std::uint16_t x, _512bits *f );
    
    static void unset_bit( std::uint16_t x, _512bits *f );


    std::uint64_t   _000_063;
    std::uint64_t   _064_127;
    std::uint64_t   _128_191;
    std::uint64_t   _192_255;
    std::uint64_t   _256_319;
    std::uint64_t   _320_383;
    std::uint64_t   _384_447;
    std::uint64_t   _448_511;
};

} /** end namespace alloc **/

#endif /* END _512BITS_HPP */
