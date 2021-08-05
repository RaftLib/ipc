/**
 * genericnode.hpp - 
 * @author: Jonathan Beard
 * @version: Thu Apr 16 07:44:03 2020
 * 
 * Copyright 2020 Jonathan Beard
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
#ifndef _GENERICNODE_HPP_
#define _GENERICNODE_HPP_  1
#include <cstdint>
#include <cassert>
#include <ostream>
#include <limits>
#include <array>
#include <type_traits>
#include <atomic> 

#include "padsize.hpp"
#include "bufferdefs.hpp"

namespace ipc
{

#pragma pack(push, 1 )
struct nodebase
{
   
    /**
     * Basically these are just number typpes, keep
     * the digits so theres a one bit hot encoding
     */
    enum node_t : std::uint8_t { dummy = 0, sentinel = 1, normal = 2, N };
    
    /** 
     * nodebase - default constructor, must have a type of some sort 
     * for this one. 
     * @param type - node_t to initialize this node to. 
     */
    constexpr nodebase( const ipc::node_id_t id,  
                        const ipc::nodebase::node_t type ) : _id( id ), 
                                                                    _type( type ){};
    
    
    /** 
     * nodebase - default constructor, must have a type of some sort 
     * for this one. 
     * @param type - node_t to initialize this node to. 
     */
    constexpr nodebase( const ipc::nodebase::node_t type ) : _type( type ){};
    
    /** 
     * nodebase - default constructor, must have a type of some sort 
     * for this one. 
     */
    constexpr nodebase( ) : _type( ipc::nodebase::dummy ){};

    using name_t = char[ 30 ];
    using name_arr_t = std::array< name_t, N >;
    /**
     * string version of the above to make things nicer for 
     * printing. 
     */
    static const  name_arr_t node_string;
    
    
    /** 
     * init_offset - default value initialization helper function
     * @return  ipc::nodebase::offset_t - proper initializaiton value.
     */
    static constexpr ipc::ptr_offset_t init_offset()
    {
        return( ipc::invalid_ptr_offset );
    }
    
    /** primarily for debugging purposes **/
    const ipc::node_id_t _id   = 0; 

    /**
     * add_link - adds child to the parent chain link, 
     * the child_offset is the relative location to the 
     * shared memory buffer data start to where the child
     * pointer is located. The parent/child ptrs are given
     * relative to the callers VA space.
     * @param parent - parent node
     * @param parent_offset - relative SHM location of parent
     * @param child  - child node
     * @param child_offset - relative SHM location of child
     */
    static void add_link(   nodebase *parent, 
                            const ipc::ptr_offset_t parent_offset,
                            nodebase *child,
                            const ipc::ptr_offset_t child_offset );


    
/**
 * looks a bit odd, but this is the only data
 * out of the entire struct that actually goes
 * inline with the struct.
 */
    node_t    _type   =   ipc::nodebase::dummy;
    //THESE WE WANT ON SAME CACHE LINE
#pragma pack( push, 1 )    
    std::atomic< ipc::ptr_offset_t >        next    =   {ipc::nodebase::init_offset()};
    ipc::ptr_offset_t                       prev    =   ipc::nodebase::init_offset();
#pragma pack( pop )    
};


/**
 * when find time, need to fix this so we can add generic compare functions. 
 * following example from 
 * https://stackoverflow.com/questions/257288/templated-check-for-the-existence-of-a-class-member-function
 */
template < typename T >
class check_compare
{
    using one = std::uint8_t;
    struct two{
        std::uint8_t x[2];
    };
    template < typename C > static one test( decltype( &C::operator == ) );
    template < typename C > static two test(...);
public:
    static constexpr bool value = 
        ( (!std::is_fundamental< T >::value) && std::is_class< T >::value && sizeof( test< T >(0) ) == sizeof( one ));
};



template < class T >
           struct alignas( L1D_CACHE_LINE_SIZE ) node : nodebase
{
    static constexpr auto padding = ipc::findpad< L1D_CACHE_LINE_SIZE, T, nodebase >::calc();
    
    constexpr node() : nodebase(){};

    constexpr node( ipc::nodebase::node_t t ): nodebase( t ){};
    
    constexpr node( const ipc::node_id_t id,  
                    const ipc::nodebase::node_t t ) : nodebase( id, t ){};
    
    template< class... Args >
    constexpr node( const ipc::node_id_t id,
                    const ipc::nodebase::node_t t,
                    Args&&... node_params ) : nodebase( id, t ),
                                              ele( std::forward< Args >( node_params )... ){}


    static_assert( padding != 0, "Something very bad has happened, struct padd + obj size cannot be zero" );
    
    /**
     * this version uses compare of the 
     * element type.
     */
    bool operator == ( const node &other )
    {
        return( other._id == (this)->_id );
    }

    /**
     * operator overload to return a reference to the 
     * contained data element (e.g., thread info object)
     */
    T& operator *() noexcept
    {
        return( ele );
    }

    T                       ele;
    std::uint8_t            _PADDING[ padding] = { 1 };
};



#pragma pack(pop)   


} /** end namespace trace_buffer **/

/**
 * cheap printme function, used primarily for debugging, 
 * however, it'd behoove anybody sub-classing this to make
 * a specific one for each subsequent type. 
 */
template < class T > std::ostream& operator << (std::ostream &stream,  ipc::node< T > &n )
{
    static const auto delim = "\t";
    stream << "Memory address of node: " << std::hex << (std::uintptr_t)(&n) << "\n";
    stream << delim << "Node type: " << ipc::nodebase::node_string[ n._type ] << "\n";
    stream << std::dec;
    stream << delim << "Node id  : " << n._id << "\n";
    stream << delim << "Node next: " << std::hex << (std::uintptr_t) n.next << "\n";
    stream << delim << "Node prev: " << std::hex << (std::uintptr_t) n.prev << "\n";
    return( stream );
}


#endif /* END _GENERICNODE_HPP_ */
