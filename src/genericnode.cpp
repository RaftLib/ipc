#include "genericnode.hpp"
#include <array>
#include <cassert>


/**
 * looks a bit odd, but this is the storage for the string
 * lookup in genericnode.hpp
 */
ipc::nodebase::name_arr_t  const ipc::nodebase::node_string
    { "Dummy", "Sentinel", "Normal" };


void
ipc::nodebase::add_link ( nodebase *parent,
                          const ipc::ptr_offset_t parent_offset,
                          nodebase *child,
                          const ipc::ptr_offset_t child_offset )
{
    assert( parent->next == ipc::nodebase::init_offset() );
    parent->next = child_offset;
    child->prev  = parent_offset;
    return;
}
