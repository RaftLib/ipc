/**
 * channelinfo_spacing.cpp - 
 * @author: Jonathan Beard
 * @version: Tue Aug  3 08:31:38 2021
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


#include "channelinfo.hpp"
#include <iostream>
#include <functional>
#include <cstdlib>
#include <cstdint>

int main()
{
    const auto ch_info_size = sizeof( ipc::channel_info );

    void *buffer = malloc( ch_info_size * 2 );

    
    auto *ch_ptr = new (buffer) ipc::channel_info( 0 );

    std::cout << "meta: "      << alignof( decltype( ch_ptr->meta ) ) << "\n";
    std::cout << "ctrl_all: "  << alignof( decltype( ch_ptr->ctrl_all ) ) << "\n";
    std::cout << "ctrl_spsc: " << alignof( decltype( ch_ptr->ctrl_spsc ) ) << "\n";
    std::cout << "spsc_q: "    << alignof( decltype( ch_ptr->spsc_q ) ) << "\n";
    

    auto convert = [&]( void *ptr, void *base ) -> auto
    {
        return( reinterpret_cast< std::uintptr_t >( ptr ) - 
                    reinterpret_cast< std::uintptr_t >( base ) );
    };
   
    //call me paranoid, but, wanna make sure alignas works
    //vs. old school struct padding.
    /**
     * NOTE: updated the check fields at bottom on 27 Aug, realized
     * that we inadvertantly set them to check for 64B cache-line 
     * alignment which fails on Apple M1 and IBM Power systems with
     * non-64B cache lines, fixed. 
     */
    std::uintptr_t diff( 0 );
    if( (diff = convert( &ch_ptr->meta, buffer      ) ) != 0 )  
    {
        std::cout << "offset should be (0), but it is (" << diff << ")\n";
        return( EXIT_FAILURE );
    }
    if( (diff = convert( &ch_ptr->ctrl_all, buffer  ) ) != (L1D_CACHE_LINE_SIZE * 4) ) 
    {
        std::cout << "offset should be (256), but it is (" << diff << ")\n";
        return( EXIT_FAILURE );
    }
    if( (diff = convert( &ch_ptr->ctrl_spsc, buffer ) ) != (L1D_CACHE_LINE_SIZE*6) ) 
    {
        std::cout << "offset should be (384), but it is (" << diff << ")\n";
        return( EXIT_FAILURE );
    }
    //this one should hit block boundary, otherwise we've broken something
    if( (diff = convert( &ch_ptr->spsc_q, buffer    ) ) != 4096 ) 
    {
        std::cout << "offset should be (4096), but it is (" << diff << ")\n";
        return( EXIT_FAILURE );
    }
    free( buffer );
    return( EXIT_SUCCESS );
}
