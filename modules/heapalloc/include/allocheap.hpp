/**
 * alloctree.hpp - 
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
#ifndef ALLOCTREE_HPP
#define ALLOCTREE_HPP  1
#include <cstdint>
#include "helper.hpp"
#include <cstring>
#include <cassert> 
#include <array>
#include <iostream>
#include <cstdlib>
#include <limits>
#include "_512bits.hpp"

namespace alloc
{

using node_index_t = std::uint16_t;

struct data_node
{
    std::uint16_t blocks_free;
    std::uint16_t contiguous_free;
    node_index_t  index;
    std::uint16_t offset_to_contiguous_free;
};


inline std::ostream& operator << ( std::ostream &stream, const data_node &n )
{
    stream << "node: \n";
    stream << "\tblocks_free: " << n.blocks_free << "\n";
    stream << "\tcontiguous_free: " << n.contiguous_free << "\n";
    stream << "\tindex: " << n.index << "\n";
    stream << "\toffset_to_free: " << n.offset_to_contiguous_free;
    return( stream );
}


template < int TotalMemSizePowerTwo /** must be an integer for power of two **/, 
           int BlockSizePowerTwo > 
#ifdef TESTHEAP
           struct 
#else
           class
#endif
           heap
{
#ifndef TESTHEAP
public:
#endif

    /** make typing faster, use this type for self **/
    using self_type = heap< TotalMemSizePowerTwo, BlockSizePowerTwo >;
    using range_t   = std::pair< std::int32_t, std::int32_t >;
    
    /** 
     * this block size is the internal leaf representation, currently
     * each "block" is a bitvector of 512 bits. If you change the size
     * of that then you need to change this number.
     */
    constexpr static std::uint8_t blocksize_pow_two  = 9;
    constexpr static std::uint16_t blocksize_bits    = (1<<blocksize_pow_two); 
    constexpr static std::uint64_t numElements = 
        1 << ( TotalMemSizePowerTwo - 9 /** 512 **/ - BlockSizePowerTwo );
    
    constexpr static std::uint64_t treeDepth   = 
        helper::find_power_of_2( numElements );
   
    heap()
    {
        std::memset( offsetArray, 0x0, sizeof( _512bits ) * self_type::numElements );
        //iterate over tree.
        for( std::uint64_t i( 0 ); i < self_type::numElements ; i++ )
        {
            self_type::add( i,this );
        }

        overall_blocks = self_type::blocksize_bits * self_type::numElements;
    }

    
    /**
     * initialize - call this if you really need to trash the heap and 
     * initialize the heap structure, 
     * when using multiple threads/processes, it is critical that 
     * this only be called ONCE (did we say that enough). The input
     * is an object of type heap. The function itself sets up the 
     * heap structures in the shared memory segment given. 
     * @param h - self_type - memory to use for heap
     */
    static void initialize( self_type *h)
    {
        std::memset( &h->offsetArray, 0x0, sizeof( _512bits ) * self_type::numElements );
        //iterate over tree.
        h->n = 0;
        for( std::uint64_t i( 0 ); i < self_type::numElements ; i++ )
        {
            self_type::add( i, h );
        }
        h->overall_blocks = self_type::blocksize_bits * self_type::numElements;
    }

    /** 
     * interface, we want to basically only show multiples of blocks, so, 
     * instead of having things like top, first, etc. for the heap, we'll
     * have functions that are more like get_n_blocks, return_n_blocks
     */

    /**
     * get_n_blocks - return offset with start + n_blocks of contiguous storage
     * that you can allocate. 
     */
    static std::int32_t get_n_blocks( const std::uint32_t n_blocks, self_type *h )
    {
        auto &top_of_heap( self_type::get_top( h ) );
        if( top_of_heap.contiguous_free >= n_blocks )
        {
            const auto bl = top_of_heap.index;
            const auto block_index_start = 
                ((1 << self_type::blocksize_pow_two) * bl) + 
                    top_of_heap.offset_to_contiguous_free; 
            
            //reset block values
            for( auto i( top_of_heap.offset_to_contiguous_free ); 
                    i < (top_of_heap.offset_to_contiguous_free + n_blocks); i++ )
            {
                _512bits::set_bit( i, &h->offsetArray[ bl ] );
            }
            self_type::updateEntry( top_of_heap, &h->offsetArray[ bl ] );
            self_type::trickleDown( 0, h );
            h->overall_blocks -=  n_blocks;
            return( block_index_start );
        }
        else
        {
            return( -1 );
        }
    }

    
    /**
     * simple constant function that returns the ceiling of the number of blocks
     * that will match nbytes within this allocator. The returned block number
     * will always return a count of blocks equal to or greater than the amount
     * of memory that was asked for. 
     * @param nbytes - literally what the param says
     * @return - ceiling of the number of blocks you will need to satisfy the 
     * nbytes requested.
     */
    static constexpr std::size_t get_block_multiple( const std::size_t nbytes )
    {
        const std::size_t multiple = ((nbytes >>  BlockSizePowerTwo ) +
                               (( nbytes & ((1 << BlockSizePowerTwo ) - 1) 
                                ) != 0 ) );
        return( multiple );                                
    }
    
    /**
     * get_blocks_avail - returns the number of blocks available
     * at the top of the heap, specificially the number of contiguous
     * blocks available. This doesn't necessarily mean that there
     * aren't more blocks available, these are just the contiguous
     * blocks. And given this is a heap organized on the most contig
     * blocks at top, this is by def the most contig blocks you can get.
     * @param h - self_type - memory with initialized heap. 
     * @return - std::size_t with the # of blocks
     */
    static std::size_t get_blocks_avail( self_type *h )
    {
        return( h->arr[ 0 ].contiguous_free );
    }

    /**
     * return_n_blocks - put the blocks back, as input you must 
     * give back the blcok offset start (given when you allocated
     * as the return, the total length of blocks allocated, 
     * and an initialized heap structure that must be the one that
     * you allocated blocks from in the first place. 
     * @param start_block - std::int64_t - block offset given from 
     * get_n_blocks
     * @param n_blocks - # of blocks given to get_n_blocks, the 
     * caller is responsible for knowing this, it won't work without
     * it. 
     * @param h - self_type - initialized heap, must be same one 
     * from the original get_n_blocks call. 
     */
    static void return_n_blocks( const std::int64_t start_block, 
                                 const std::int64_t n_blocks, 
                                 self_type *h )
    {
        //get index (integer div)
        const auto index = start_block / self_type::blocksize_bits;
        //get offset (mod)
        const auto offset_start = start_block % self_type::blocksize_bits;
        //unset bits
        for( auto i( offset_start ); i < (offset_start+n_blocks); i++ )
        {
            alloc::_512bits::unset_bit( i, &h->offsetArray[ index ] );
        }
        //search backwards through the list
        for( std::int64_t j( self_type::numElements - 1 ); j >= 0; j-- )
        {
            if( h->arr[ j ].index == index )
            {
                self_type::updateEntry( h->arr[ j ], &h->offsetArray[ index ] );
                self_type::bubbleUp( j, h );
                break;
            }
            else
            {
                continue;
            }
        }
        //trickle down head
        self_type::trickleDown( 0, h );
        h->overall_blocks +=  n_blocks;
    }
    
    /**
     * get_current_free - do not use this to figure out 
     * what you can allocate, this is simply used to return
     * the overall status of the heap and number of blocks
     * available (as in total blocks free, not necessarily 
     * allocatable contiguous ranges).
     * @param self_type - heap pointer
     * @return overall free size of this heap.
     */
    static std::size_t get_current_free( self_type *h ) 
    {
        return( h->overall_blocks );
    }

#ifndef TESTHEAP
private:
#endif
    static constexpr node_index_t  parent( const node_index_t current_index )
    {
        return( ( current_index - 1 ) >> 1 );
    }

    static constexpr node_index_t  go_left( const node_index_t current_index )
    {
        return( (2 * current_index) + 1 );
    }

    static constexpr node_index_t go_right( const node_index_t current_index )
    {
        return( (2 * current_index) + 2 );
    }

    /**
     * compare - function to set bubble-up, will set to 
     * bubble up blocks with the most contiguous sections
     * to start off with, but if you'd want to change then
     * do so here. 
     * @param a - index of first element
     * @param b - idnex of second element
     * @param h - heap structure
     * @return - <1 if a is less, >1 if a is greater, 0 if 
     * a==b.
     */
    static constexpr std::int32_t compare( const node_index_t a,
                                           const node_index_t b,
                                           const self_type * const h ) noexcept 
    {
        const auto val_a = h->arr[ a ].contiguous_free;
        const auto val_b = h->arr[ b ].contiguous_free;
        if( val_a < val_b )
        {
            return( -1 );
        }
        else if( val_a > val_b )
        {
            return( 1 );
        }
        else
        {
        //( val_a == val_b )
            return( 0 );
        }
    }

    static void swap( const node_index_t a,
                      const node_index_t b,
                      self_type *h )
    {
        const auto temp_a = h->arr[ a ];
        h->arr[ a ] = h->arr[ b ];
        h->arr[ b ] = temp_a;
        return;
    }


    /**
     * bubbleUp - bubble up this index (current_index) to where
     * it is no longer less than the parent 
     * @param - current_index - data node to be bubbled up
     * @param - h - heap pointer 
     */
    static void bubbleUp(   node_index_t current_index, 
                            self_type *h )
    {
        for( auto p = self_type::parent( current_index ); 
                (current_index > 0 /** not inserting first node **/ &&  
                    self_type::compare( current_index, p, h ) > 0 ); 
                        current_index = p, 
                        p = self_type::parent( current_index ) )
        {
            self_type::swap( current_index, p, h );
        }
        return;
    }

    static void updateEntry( data_node &node_data, alloc::_512bits *entry )
    {
        assert( entry != nullptr );
        node_data.blocks_free   = blocksize_bits - _512bits::total_bits_set( entry );
        const auto p            = alloc::_512bits::find_longest_contiguous_zeros( entry ); 

        node_data.contiguous_free           = p.first;
        node_data.offset_to_contiguous_free = p.second;
        return;
    }

    static bool add( const node_index_t entry_index, self_type *h )
    {
        if( h->n + 1 > ( 1 << self_type::treeDepth ) )
        {
            //FIXME - add proper assertion here
            return( false );
        }
        //else
        auto *entry     = &h->offsetArray[ entry_index ];
        auto &node_data = h->arr[ h->n ];
        node_data.index = entry_index;
        //get number of zeros totalbits - ones
        self_type::updateEntry( node_data, entry );
        h->n++;
        self_type::bubbleUp( h->n - 1, h );
        return( true );
    }

    static data_node&   get_top( self_type *h )
    {
        return( h->arr[ 0 ] );
    }

    static void trickleDown( std::int32_t i, self_type *h ) 
    {
        do{
            auto j = -1;
            auto r = self_type::go_right( i );
            if( r < h->n && self_type::compare(  r, i, h ) > 0 )
            {
                auto l = self_type::go_left( i );
                if( self_type::compare( l, r, h ) > 0 ) 
                {
                    j = l;
                } 
                else 
                {
                    j = r;
                }
            } 
            else 
            {
                auto l = self_type::go_left( i );
                if( l < h->n && self_type::compare(  l, i, h ) > 0 ) 
                {
                    j = l;
                }
            }
            if( j >= 0 )
            {   
                self_type::swap( i, j, h );
            }
            i = j;
        } while( i >= 0 );
    }


//data
    data_node       arr[ 1 << treeDepth ];
    _512bits        offsetArray[ numElements ]; 
    std::int32_t    n               = 0;
    std::size_t     overall_blocks  = 0;
};


} /** end namespace alloc **/


#endif /* END ALLOCTREE_HPP */
