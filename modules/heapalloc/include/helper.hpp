#ifndef HELPER
#define HELPER 1
#include <cstdint>
#include <climits>
#include <limits>
#include <algorithm>
#include <iostream>

namespace helper
{

constexpr std::int32_t  find_power_of_2( std::uint64_t x) 
{
    std::uint64_t z = 0x0000000100000000ULL;
    std::int32_t  p = 31, d = 16;
    while (d) 
    {
        if (x & (z-1)) 
        {
            p -= d;
            z >>= d;
        }
        else 
        {
            p += d;
            z <<= d;
        }
        d >>= 1;
    }
    return( x ? ((x & z) ? p+1 : p) : -1 );
}


/**
 * find_longest_contiguous_zeros - simple function to find the 
 * longest contiguous stretch of zeros. This isn't the most 
 * efficient way to do it (could do binary search), but...
 * likely not going to be a bottleneck so we'll stick with the
 * simple version for now. 
 * @param val - std::uint64_t value to find zero stretch in
 * @return - longest zero stretch + offset for start
 */
inline std::pair< std::int32_t, std::int32_t > 
find_longest_contiguous_zeros( std::uint64_t val  ) 
{ 
    if (~val == std::numeric_limits< std::uint64_t >::max() )
    {
        return( std::make_pair( 0, 0 ) );
    }
    //mask off everything but bottom bit
    constexpr static std::uint64_t mask = (1ULL);

    std::int32_t    currLen         = 0, 
                    maxLen          = 0, 
                    maxOffsetStart  = 0, 
                    returnOffset    = 0;

    for( std::int32_t position = 0, i = 0; i < 64; val = val >> 1, position++, i++ ) 
    {
        const auto curr_bit = val & mask;
        std::cout << curr_bit << "\n";
        if( curr_bit == 0 )
        {
            currLen++;
            maxOffsetStart = position;
        }
        else
        {
            if( currLen > maxLen )
            {
                maxLen       = currLen;
                returnOffset = maxOffsetStart;
            }
            currLen = 0;
        }
    }
    if( currLen > maxLen )
    {
        maxLen       = currLen;
        returnOffset = maxOffsetStart;
    }
    return( std::make_pair( maxLen, returnOffset )  );
} 

} /** end namespace helper **/

#endif /** end HELPER **/
