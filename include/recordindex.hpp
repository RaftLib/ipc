/**
 * record_index.hpp - 
 * @author: Jonathan Beard
 * @version: Thu Apr 16 10:32:36 2020
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
#ifndef _RECORDINDEX_HPP_
#define _RECORDINDEX_HPP_  1

#include "indexbase.hpp"
#include "bufferdefs.hpp"

namespace ipc
{
  /**
   * basically we have a generic node type here, 
   * that has inserted within it a data structure
   * that has head/tail pointers into the async
   * FIFO that contains data for each thread. 
   */
  using record_index_t = index_t< ipc::ptr_offset_t >;   
} /** end namespace ipc **/

#endif /* END _RECORDINDEX_HPP_ */
