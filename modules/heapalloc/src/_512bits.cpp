/**
 * _512bits.cpp - 
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
#include "_512bits.hpp"
#include <limits>
#include <cassert>

std::uint16_t 
alloc::_512bits::total_bits_set( _512bits *f )
{
    return( __builtin_popcountl( f->_000_063 ) + 
            __builtin_popcountl( f->_064_127 ) +
            __builtin_popcountl( f->_128_191 ) +
            __builtin_popcountl( f->_192_255 ) +
            __builtin_popcountl( f->_256_319 ) +
            __builtin_popcountl( f->_320_383 ) +
            __builtin_popcountl( f->_384_447 ) +
            __builtin_popcountl( f->_448_511 ) );
}
    

std::pair< std::int32_t, std::int32_t > 
alloc::_512bits::find_longest_contiguous_zeros( _512bits *f )
{
    const auto total_bits_flipped = _512bits::total_bits_set( f );
    if(  total_bits_flipped == 512 )
    {
        return( std::make_pair( 0 /** longest stretch **/, 0 /** offset of setretch **/) );
    }
    else if( total_bits_flipped == 0 )
    {
        return( std::make_pair( 512 /** longest stretch **/, 0 /** start **/ ) );
    }
    //else we have to do something more complicated
    
    //mask off everything but bottom bit
    constexpr static std::uint64_t mask = (1ULL<<63);
    
    std::int32_t    currLen         =   0, 
                    currOffset      =   0, 
                    maxLen          =   0, 
                    maxOffsetStart  =   0;

    int slice_start = 0;
    std::uint64_t *curr_slice = nullptr;
    for( int slice_position = 511; slice_position >=0; slice_position-=64 )
    {
        if( slice_position < 64 )
        {
            curr_slice = &f->_000_063;
            slice_start = 0;
        }
        else if( slice_position < 128 )
        {
            curr_slice = &f->_064_127; 
            slice_start = 64;
        }
        else if( slice_position < 192 )
        {
            curr_slice = &f->_128_191;
            slice_start = 128;
        }
        else if( slice_position < 256 )
        {
            curr_slice = &f->_192_255;
            slice_start = 192;
        }
        else if( slice_position < 320 )
        {
            curr_slice = &f->_256_319;
            slice_start = 256;
        }
        else if( slice_position < 384 )
        {
            curr_slice = &f->_320_383; 
            slice_start = 320;
        }
        else if( slice_position < 448 )
        {
            curr_slice = &f->_384_447;
            slice_start = 384;
        }
        else if( slice_position < 512 )
        {
            curr_slice = &f->_448_511;
            slice_start = 448;
        }
        //make copy
        auto val = *curr_slice;
        if (~(val) == std::numeric_limits< std::uint64_t >::max() )
        {
            //all zeros
            currLen += 64;
            currOffset = slice_start;
        }
        else if ( val == std::numeric_limits< std::uint64_t >::max() )
        {
            //all ones
            if( currLen > maxLen )
            {
                maxLen          = currLen;
                maxOffsetStart  = currOffset;
            }
            currLen = 0;
        }
        else
        {
            //else, we need to process this slice
            for( std::int32_t position = 63; position >= 0; val = val << 1, position-- ) 
            {
                const auto curr_bit = val & mask;
                if( curr_bit == 0 )
                {
                    currLen++;
                    currOffset = (position + slice_start);
                }
                else
                {
                    if( currLen > maxLen )
                    {
                        maxLen          = currLen;
                        maxOffsetStart  = currOffset;
                    }
                    currLen = 0;
                }
            }
        }
    
    }
    if( currLen > maxLen )
    {
        maxLen          = currLen;
        maxOffsetStart  = currOffset;
    }
    return( std::make_pair( maxLen, maxOffsetStart )  );
}
    
void 
alloc::_512bits::set_bit( std::uint16_t x, _512bits *f )
{
    if( x < 64 )
    {
       f->_000_063 |= (1ULL << x);
    }
    else if( x < 128 )
    {
        f->_064_127 |= (1ULL << x);

    }
    else if( x < 192 )
    {
        f->_128_191 |= (1ULL << x);
    }
    else if( x < 256 )
    {
        f->_192_255 |= (1ULL << x);
    }
    else if( x < 320 )
    {
        f->_256_319 |= (1ULL << x);
    }
    else if( x < 384 )
    {
        f->_320_383 |= (1ULL << x);

    }
    else if( x < 448 )
    {
        f->_384_447 |= (1ULL << x);

    }
    else if( x < 512 )
    {
        f->_448_511 |= (1ULL << x);
    }
    return;
}

void 
alloc::_512bits::unset_bit( std::uint16_t x, _512bits *f )
{
    if( x < 64 )
    {
       f->_000_063 &= ~(1ULL << x);
    }
    else if( x < 128 )
    {
        f->_064_127 &= ~(1ULL << x);

    }
    else if( x < 192 )
    {
        f->_128_191 &= ~(1ULL << x);
    }
    else if( x < 256 )
    {
        f->_192_255 &= ~(1ULL << x);
    }
    else if( x < 320 )
    {
        f->_256_319 &= ~(1ULL << x);
    }
    else if( x < 384 )
    {
        f->_320_383 &= ~(1ULL << x);
    }
    else if( x < 448 )
    {
        f->_384_447 &= ~(1ULL << x);
    }
    else if( x < 512 )
    {
        f->_448_511 &= ~(1ULL << x);
    }
    return;
}
