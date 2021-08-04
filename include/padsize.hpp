/**
 * padsize.hpp - 
 * @author: Jonathan Beard
 * @version: Tue Apr 21 13:02:38 2020
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
#ifndef _PADSIZE_HPP_
#define _PADSIZE_HPP_  1

#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <type_traits>
#include <typeinfo>
#include <array>
#include <cstddef>
#include <cmath>

namespace ipc
{
    //predeclare this struct
    template < class... TYPES > struct _calc;

    /**
     * findpad - this struct is what is called by the 
     * end user, well, the static function at least. 
     */
    template < std::size_t DESIREDSIZE, class... TYPES > struct findpad
    {   
        /**
         * calc - call this to get the padding required to align
         * an object with all the types listed above to the DESIREDSIZE
         * from the template. Basically I just got tired of re-writing this
         * over and over in a non-portable say so I just wrote it 
         * as a template metaprogram.
         * @return std::uint64_t with the number of bytes that need to be
         * added to fill to the desired size. It should be noted this 
         * does not fix start byte alignment, that's up to user ingenuity :)
         */
        static constexpr std::uint64_t calc()
        {
            const auto type_sizes( ipc::_calc< TYPES... >::sizes() );
            if( type_sizes == 0 || type_sizes == DESIREDSIZE )
            { 
                return( 0 ); 
            }
            else if( type_sizes > DESIREDSIZE )
            {
                //can't do anything nice, but...we can return a multiple of DESIREDSIZE
                const auto multiple( (type_sizes / DESIREDSIZE) + ((type_sizes % DESIREDSIZE) != 0) );
                return( (DESIREDSIZE * multiple) - type_sizes );
            }
            else
            {
                return( DESIREDSIZE - type_sizes );
            }
        }
    };


    /**
     * main recursive struct
     */
    template < class T, class... TYPES > struct _calc < T, TYPES... >
    {
        _calc()  = delete;
        ~_calc() = delete;
        
        /** 
         * padding - recursive function to _calculate the
         * amount of padding to add to a structure to 
         * match it to the alignment required.
         */
        static constexpr std::uint64_t sizes()
        {
            return( sizeof( T ) + _calc< TYPES... >::sizes() );
        }
    };
    
    /**
     * template recursion base case
     */
    template <> struct _calc<>
    {
        _calc()  = delete;
        ~_calc() = delete;
        /**
         * sizes - recursive function base case, will 
         * return 0;
         */
        static constexpr std::uint64_t sizes(){ return( 0 ); }

    };

} /** end namespace ipc **/

#endif /* END _PADSIZE_HPP_ */
