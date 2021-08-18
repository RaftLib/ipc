/**
 * buffer.hpp - 
 * @author: Jonathan Beard
 * @version: Fri Mar 13 09:35:16 2020
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
#ifndef _BUFFER_HPP_
#define _BUFFER_HPP_  1

#include <cstdint>
#include <string>
#include <sstream>
#include "genericnode.hpp"
#include "indexbase.hpp"
#include "database.hpp"
#include "threadlocaldata.hpp"
#include "bufferdefs.hpp"

#include "buffer_base.hpp"
    


namespace ipc
{

struct global_err_t
{
    std::stringstream err_msg;
    ipc::buffer       *buffer = nullptr;
    std::string       shm_handle;
};



class buffer :
               public buffer_base

{
private:
    static global_err_t gb_err;

    /**
     * err handling func
     */
    static void shutdown_handler( int signum );

    
    /** 
     * set global block inc to be 1MiB, will allocate multiple 'x'
     * of this if the user requests large blocks.
     */
    static constexpr std::size_t global_block_inc = (1 << 20);

    static std::string get_tmp_dir();

    static void* thread_local_allocate( ipc::thread_local_data *data, 
                                        const std::size_t blocks,
                                        const ipc::channel_id_t channel_id );

    static void* global_buffer_allocate( ipc::thread_local_data *data, 
                                         std::size_t &blocks,
                                         bool        force_size = false );
   
    /**
     * create_meta_record - create a per-allocate (per local thread allocate) 
     * record that contains critical information about the specific alloation,
     * will be created/initialized on a hidden page above the given user
     * allocation. This function will increment the free pointer and available 
     * bytes accordingly. Please ensure the function that calls this one
     * checks for the block multiple of the meta data struct + the requested
     * user allocation before proceeding.
     */
    static void 
    create_meta_record( void                    *data_buffer_start,
                        ipc::ptr_offset_t       &allocation_base,
                        std::size_t             &blocks_avail,
                        const std::size_t       multiple );
    
    /**
     * find_channel_buffer_offset - 
     * this is an internal version that does not 
     * grab the semaphore before searching the channel list, it 
     * is what is called by the main version of find_channel.
     * The parameter channel is set to the channel_info object,
     * and the returned value is the physical offset within the 
     * buffer that this channel is found at. If an error occurs,
     * then 
     */
    static 
    ipc::ptr_offset_t find_channel_buffer_offset( ipc::thread_local_data *tls, 
                                                  const channel_id_t      channel_id,
                                                  ipc::channel_info       **channel );
    
    /**
     * find_channel_block - returns the actual physical index
     * of the channel index object that represents the channel. 
     * @param - valid data struct
     * @param - channel_id - channel that you want the index for
     * @return ptr_offset_t, if valid then its >= ipc::valid_offset,
     * else it will be set as follows:
     * ipc::ptr_not_found -> channel not found
     */
    static ipc::ptr_offset_t
    find_channel_block(  ipc::thread_local_data *data,
                         const channel_id_t      channel_id );
    
    /**
     * _free - this function takes in a block base (with respect to 
     * modulo block_size) and the number of blocks to free, then 
     * returns these blocks to the heap. This function gets a sem
     * before doing any actual work, so beware that the sem_alloc
     * will be locked here.
     * @param data - valid TLS segment
     * @param block_base - base with respect to heap (mod block size)
     * @param blocks - total count of blocks (to include metadata) that 
     * you want to return to the heap.
     */
    static void _free( ipc::thread_local_data *data,
                       const ipc::ptr_offset_t block_base,
                       const std::size_t blocks);

    static void free_channel_memory( ipc::thread_local_data *data, ipc::local_allocation_info &info );
    
    static
    channel_id_t add_channel( ipc::thread_local_data *tls, 
                              const channel_id_t channel_id,
                              const ipc::channel_type  channel_type );
                                        
public:
    buffer();
    ~buffer() = default;


    /**
     * allocate - allocate nbytes of data from the buffer, if not enough 
     * contiguous blocks are left over, nullptr is returned. Once you allocate
     * you should call ``send'' to send the buffer to the channel. You should
     * not call allocate again until you do so, this will result in memory
     * leaks. 
     * @param   data    - thread local data struct
     * @param   nbytes  - number of bytes that you wish to allocate.
     * @param   channel_id - channel that the caller wants to allocate mem on.
     * @return - valid pointer if successful, nullptr if not. 
     */
    static void* allocate_record( ipc::thread_local_data *data, 
                           const std::size_t nbytes,
                           const ipc::channel_id_t channel_id );

    /**
     * free - deallocate a previously allocated block of data.
     * @param   data   - thread local data struct, can be initialized 
     * for each thread by calling "get_tls_structure" and closed by calling
     * "close_tls_structure". 
     * @param   data    - thread local data struct
     * @param   ptr     - VA from caller space, will be converted to offset 
     * in buffer then to the receivers VA space. 
     */
    static void free_record( ipc::thread_local_data *data,
                             void *ptr );
   

    /**
     * receive_record - receive a buffer, attach to the calling TLS
     * space to use locally. Once received, the local thread
     * is responsible for deallocating it back to the main
     * buffer using ipc::free. 
     * @param tls_data - thread local struct belonging to the 
     * thread and address space to which you want the memory
     * attached. 
     * @param channel_id - channel to which you want to receive
     * data from. 
     * @return code - ipc::tx_success on successful return, 
     * otherwise check error codes. 
     */
    static ipc::tx_code send_record( ipc::thread_local_data *tls_data,
                                     const ipc::channel_id_t channel_id,
                                     void **record );
    
    /**
     * receive_record - receive a buffer, attach to the calling TLS
     * space to use locally. Once received, the local thread
     * is responsible for deallocating it back to the main
     * buffer using ipc::free. 
     * @param tls_data - thread local struct belonging to the 
     * thread and address space to which you want the memory
     * attached. 
     * @param channel_id - channel to which you want to receive
     * data from. 
     * @return code - ipc::tx_success on successful return, 
     * otherwise check error codes. 
     */
    static ipc::tx_code receive_record( ipc::thread_local_data *tls_data,
                                        const ipc::channel_id_t channel_id,
                                        void **record );



    static ipc::thread_local_data* get_tls_structure( ipc::buffer *buffer,
                                                      const ipc::thread_id_t thread_id );


    static void close_tls_structure( ipc::thread_local_data *d );

    /**
     * initialize - will initialize the structure and return a ipc
     * object. If you're not the first person to call this function
     * that's perfectly fine, you'll get back the structure some
     * other person set up...so basically you can call this object
     * over and over and it won't make a mess. This should be called once
     * per process (per buffer).
     * @param   shm_handle - string handle to initialize shm_handle to
     * @param   buffer_size_nbytes - planned size of data buffer (not including
     *          the struct itself)
     * @return  ipc::buffer* object, fully initialized and ready to go
     * @throws - see shm header file for errors.
     */
    static
    ipc::buffer*   initialize( const std::string shm_handle );


    /**
     * destruct - will unmap the memory for the buffer b and optionally unlink
     * b's semaphores and unlink the key for the b. This should be
     * called with unlink once per process (per buffer) (see caveat 
     * for unlink). Otherwise the memory and associated stuff with this
     * buffer will be unmapped from the callers  address space. 
     * @param   b - the buffer to be destroyed
     * @param   shm_handle - the string handle associated with b
     * @return  ipc::buffer* object, fully initialized and ready to go
     * @throws - see shm header file for errors.
     */
    static
    void                    destruct( ipc::buffer *b,
                                      const std::string &shm_handle,
                                      const bool unlink = true );
    /**
     * add_channel - this function could be called per thread to 
     * add a channel to the local thread context. If the channel
     * already exists in the global context then this channel
     * is added to the local thread context with a zero allocation
     * (zero given this thread has yet to call allocate on this 
     * channel). Call allocate once you have channels added to 
     * get some memory. 
     * @param   tls - allocated and valid thread_local_data structure
     * @param   channel_id - id of channel you want to add to this
     * thread context. If it doesn't exist, it will be created for you
     * @return channel id that was added if successful. Returns error
     * codes found in bufferdata.hpp if not successful. 
     * specific error codes
     * ipc::null_channel - something very bad happened, plz check code
     * ipc::channel_alloc_err - memory for channel could not be 
     * allocated
     * ipc::channel_err - channel could not be inserted due to some
     * error. 
     */
    //static
    //channel_id_t add_mn_lf_channel( ipc::thread_local_data *tls, 
    //                                const channel_id_t channel_id );
    
    /**
     * add_spsc_lf_record - this function could be called per thread to 
     * add a recordchannel to the lacal thread context. Record channels
     * are designed for use cases where mixed-object types are needed of 
     * varying size requirements on a single channel. As an example, if 
     * you have files that you need to fill that are 4KiB, 16KiB, 1MiB, and
     * all over the same channel, and you want zero copy, then this is for you. 
     * If the channel you want to initialize 
     * already exists in the global context then this channel
     * is added to the local thread context with a zero allocation
     * (zero given this thread has yet to call allocate on this 
     * channel). Call allocate once you have channels added to 
     * get some memory. This channels should be used with the corresponding
     * "_record" function calls. 
     * @param   tls - allocated and valid thread_local_data structure
     * @param   channel_id - id of channel you want to add to this
     * thread context. If it doesn't exist, it will be created for you
     * @return channel id that was added if successful. Returns error
     * codes found in bufferdata.hpp if not successful. 
     * specific error codes
     * ipc::null_channel - something very bad happened, plz check code
     * ipc::channel_alloc_err - memory for channel could not be 
     * allocated
     * ipc::channel_err - channel could not be inserted due to some
     * error. 
     */
    static
    channel_id_t add_spsc_lf_record_channel( ipc::thread_local_data *tls, 
                                             const channel_id_t channel_id );
    

    /**
     * add_spsc_lf_data_channel - unlike the record channel, this channel
     * for a specific single type/size for both the producer consumer 
     * side. 
     */
    template < class T >
    static
    channel_id_t    add_spsc_lf_data_channel( ipc::thread_local_data *tls,
                                              const channel_id_t channel_id )
    {
        UNUSED( tls );
        UNUSED( channel_id );
        //do some stuff
        return( 0 );
    }

    /**
     * find_channel - returns the information about the target 
     * channel, specifically you give as parameters the TLS and 
     * desired channel ID, and if that channel exists it will set
     * the pointer channel to the desired channel structure. 
     */
    static 
    channel_id_t find_channel( ipc::thread_local_data *tls, 
                                const channel_id_t      channel_id,
                                ipc::channel_info       **channel );
    /**
     * remove_channel - remove channel with given ID assicoated with this tls
     * structure.
     * @return bool - if channel found, if found it's refcount is decremented
     * if refcount is zero, channel is removed and deallocated to buffer.
     */
    static
    bool                  remove_channel( ipc::thread_local_data *tls,
                                          const channel_id_t channel_id );
    

    /**
     * unlink_channels - spins through tls structure and decrements the 
     * refcount on each channel referenced by this thread. If the refcount
     * is zero, the memory for the channel is deallocated and returned to 
     * the buffer pool. 
     */
    static
    void           unlink_channels( ipc::thread_local_data *tls );

    /**
     * IMPORTANT, KEEP LAST
     * the buffer_base struct is laid out before this one and is block_size aligned.
     */
    alignas( 1<< ipc::block_size_power_two ) ipc::byte_t           data[ 1 ];

};

} /** end namespace ipc **/
#endif /* END _BUFFER_HPP_ */
