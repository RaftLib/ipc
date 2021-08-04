#include <cstdint>
#include <iostream>
#include <cstdlib>
#include <cassert>
#include <limits>

#define TESTHEAP 1
#include "allocheap.hpp"

int main()
{
    using heap_t = alloc::heap< 30, 12 >;
    
    if( heap_t::numElements != (1 << (30 - 12 - 9) ) )
    {
        exit( EXIT_FAILURE );
    }
    
    heap_t theheap; 

    //test parent 
    if( heap_t::parent( 1 ) != 0 )
	{
        exit( EXIT_FAILURE );
	}
    if( heap_t::parent( 2 ) != 0 )
	{
        exit( EXIT_FAILURE );
	}
    if( heap_t::parent( 3 ) != 1 )
	{
        exit( EXIT_FAILURE );
	}
    if( heap_t::parent( 4 ) != 1 )
	{
        exit( EXIT_FAILURE );
	}
    if( heap_t::parent( 5 ) != 2 )
	{
        exit( EXIT_FAILURE );
	}
    if( heap_t::parent( 6 ) != 2 )
	{
        exit( EXIT_FAILURE );
	}


    //test left/right functions 
    if( heap_t::go_right( 0 ) != 2 )
	{
        exit( EXIT_FAILURE );
	}
    if( heap_t::go_left( 0 )  != 1 )
	{
        exit( EXIT_FAILURE );
	}

    if( heap_t::go_right( 1 ) != 4 )
	{
        exit( EXIT_FAILURE );
	}
    if( heap_t::go_left( 1 )  != 3 )
	{
        exit( EXIT_FAILURE );
	}
    
    /** set first two to make sure we haven't hosed something basic **/
    alloc::_512bits::set_bit( 1, &theheap.offsetArray[ 0 ] );
    alloc::_512bits::set_bit( 0, &theheap.offsetArray[ 0 ] );
    assert( alloc::_512bits::total_bits_set(  &theheap.offsetArray[ 0 ] ) == 2 );
   
    /** set the last two bits, ensure we have the right width on the integers to set **/
    alloc::_512bits::set_bit( 511, &theheap.offsetArray[ 0 ] );
    alloc::_512bits::set_bit( 510, &theheap.offsetArray[ 0 ] );
    assert( alloc::_512bits::total_bits_set(  &theheap.offsetArray[ 0 ] ) == 4 );
    
    /** unset last two bits in set, see if it equals two **/
    alloc::_512bits::unset_bit( 511, &theheap.offsetArray[ 0 ] );
    alloc::_512bits::unset_bit( 510, &theheap.offsetArray[ 0 ] );
    assert( alloc::_512bits::total_bits_set(  &theheap.offsetArray[ 0 ] ) == 2 );
   
    /** calling unset again should have no effect **/
    alloc::_512bits::unset_bit( 511, &theheap.offsetArray[ 0 ] );
    alloc::_512bits::unset_bit( 510, &theheap.offsetArray[ 0 ] );
    assert( alloc::_512bits::total_bits_set(  &theheap.offsetArray[ 0 ] ) == 2 );


    
    

    auto ret_val = alloc::_512bits::find_longest_contiguous_zeros( &theheap.offsetArray[ 0 ] );
    if( ret_val.first != 510 && ret_val.second != 2 )
    {
        return( EXIT_FAILURE );
    }

    //test some known patterns
    auto *harr_1 = &theheap.offsetArray[ 1 ];
    for( auto i( 0 ); i <= 399; i++ )
    {
        alloc::_512bits::set_bit( i, harr_1 );
    }
    auto ret_val2 = alloc::_512bits::find_longest_contiguous_zeros( harr_1 );
    if( ret_val2.first != 112 && ret_val2.second != 400 )
    {
        return( EXIT_FAILURE );
    }
    
    //unset pattern
    for( auto i( 0 ); i <= 512; i++ )
    {
        alloc::_512bits::unset_bit( i, harr_1 );
    }
    //set new pattern
    for( auto i( 0 ); i <= 255; i++ )
    {
        alloc::_512bits::set_bit( i, harr_1 );
    }
    auto ret_val3 = alloc::_512bits::find_longest_contiguous_zeros( harr_1 );
    if( ret_val3.first != 256 && ret_val3.second != 256 )
    {
        return( EXIT_FAILURE );
    }
    
    
    //unset pattern
    for( auto i( 0 ); i <= 512; i++ )
    {
        alloc::_512bits::unset_bit( i, harr_1 );
    }
    ret_val3 = alloc::_512bits::find_longest_contiguous_zeros( harr_1 );
    if( ret_val3.first != 512 && ret_val3.second != 0 )
    {
        return( EXIT_FAILURE );
    }

    
    const auto val = heap_t::get_n_blocks( 10, &theheap );
    const auto val2 = heap_t::get_n_blocks( 10, &theheap );
    

    heap_t::return_n_blocks( val, 10, &theheap );
    heap_t::return_n_blocks( val2, 10, &theheap );

    //check to see what happens when it's "full", we have 512 blocks of 512
    for( auto i( 0 ); i < 512; i++ )
    {
        if( heap_t::get_blocks_avail( &theheap ) != 512 )
        {
            return( EXIT_FAILURE );
        }
        else
        {
            heap_t::get_n_blocks( 512, &theheap );
        }
    }
    for( std::uint64_t i( 0 ); i < heap_t::numElements; i++ )
    {
        if( theheap.arr[ i ].contiguous_free != 0 )
        {
            return( EXIT_FAILURE );
        }
    }
    
    //let's return all 
    for( std::uint64_t i( 0 ); i < (heap_t::numElements*512); i+= 512 )
    {
        heap_t::return_n_blocks( i, 512, &theheap );
    }
    for( std::uint64_t i( 0 ); i < heap_t::numElements; i++ )
    {
        if( theheap.arr[ i ].contiguous_free != 512 )
        {
            return( EXIT_FAILURE );
        }
    }


    std::cout << "SUCCESS\n";
    return( EXIT_SUCCESS );
}
