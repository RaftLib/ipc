/**
 * indexbase.hpp - 
 * @author: Jonathan Beard
 * @version: Thu Apr 16 07:31:29 2020
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
#ifndef _INDEXBASE_HPP_
#define _INDEXBASE_HPP_  1

#include "genericnode.hpp"

namespace ipc
{

/**
 * NOTE: as a design point we could have included here
 * a type that would index within the larger data structure
 * directly, instead to make this a bit more flexible, 
 * you have to create a class and inject the type into
 * index_t. This is more flexible b/c it allows the 
 * end developer to add any time of data structure within 
 * the data segment and custom indexing. For an async fifo
 * you only need head/tail pointers, but for other structures
 * you might need something else. 
 */
template < class T > using index_t  = ipc::node< T >;

} /** end namespace ipc **/
#endif /* END _INDEXBASE_HPP_ */
