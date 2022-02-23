/**
 * ch_data_spsc.hpp - 
 * @author: Jonathan Beard
 * @version: Mon Aug  2 13:18:51 2021
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
#ifndef CH_DATA_SPSC_HPP
#define CH_DATA_SPSC_HPP  1

#include "bufferdefs.hpp"

namespace ipc
{

template < class T, int n_entries >
struct alignas( (1<<ipc::block_size_power_two) ) ch_data_spsc
{
    
    constexpr ch_data_spsc()
    {
     
    };

    T entry[ n_entries ] = { reinterpret_cast< T >( 0 ) };

}; 

} /** end namespace ipc **/

#endif /* END CH_DATA_SPSC_HPP */
