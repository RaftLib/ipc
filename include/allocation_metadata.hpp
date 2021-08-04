#ifndef _ALLOCATE_METADATA_HPP_
#define _ALLOCATE_METADATA_HPP_  1
#include <cstdint>
#include <cstddef>
#include <unistd.h>
#include <cassert>
#include "bufferdefs.hpp"
#include "padsize.hpp"
#if DEBUG
#include <limits>
#endif 

namespace ipc
{

struct allocate_metadata
{

    allocate_metadata  ( const ipc::ptr_offset_t base, 
                         const std::size_t       block_count ) : base_block_offset( base ),
                                                                block_count(  block_count ){};

    ipc::ptr_offset_t   base_block_offset    = 0;
    std::size_t         block_count          = 0;

    ipc::byte_t         padding[ ipc::findpad< ipc::block_size_power_two /** total size we want **/,
                                               ipc::ptr_offset_t, 
                                               std::size_t >::calc() ]


#if DEBUG
    /** scribble with all '1' to make it easier to debug **/
    = { std::numeric_limits< ipc::byte_t >::max() };
#else
;
#endif
};

} /** end namespace ipc **/

#endif /* END _ALLOCATE_METADATA_HPP_ */
