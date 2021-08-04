
void*
ipc::buffer::global_buffer_allocate( 
                       ipc::thread_local_data *data,
                       const std::size_t      nbytes )
{
    /**
     * if we're here we need to go out to the global buffer,
     * then the calling function can call the thread local
     * allocate function.
     */

#ifdef DEBUG
    // This is used for testing the semaphore
    // We want both threads to get here and wait for the main thread to release them.
    while( data->buffer->spin_lock )
    {
        __asm__ volatile( "nop" : : : );
    }
#endif

    //get thread local version of allocate semaphore
    /** LOCK **/
    auto *sem = data->allocate_semaphore;
    if( sem_wait( sem ) != 0 )
    {
        std::perror( "Failed to wait on semaphore to allocate, aborting allocate!" );
        return( nullptr );
    }

#if DEBUG
    // Atomically increment a variable indicating that a thread is in the critical section
    if( std::atomic_fetch_add(&data->buffer->critical_count, 1) > 1 )
    {
        std::perror( "Too many threads in critical section" );
        return (nullptr);
    }
#endif

    // The first thread that gets to the semaphore needs to wait to make sure the other
    // thread has time to wait on it. The second thread will also end up waiting here
    // but that's fine.
#if DEBUG
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(0.125s);
    }
#endif


    
    /**
     * STEP 1: calculate the multiple of increment size to allocate here.
     */
    const auto meta_blocks      = heap_t::get_block_multiple( sizeof( ipc::allocate_metadata ) );
    const auto data_multiple    = heap_t::get_block_multiple( nbytes ); 
    
    const auto multiple = meta_blocks + data_multiple;

    /**
     * STEP 2: initialize output pointer.
     */
    void *output = nullptr;

    if( multiple <= ipc::meta_info::heap_t::get_blocks_avail( &data->buffer->heap  ) )
    {
        /**
         * STEP 3: set start pointer to the current free start
         */
        const auto block_base = ipc::meta_info::heap_t::get_n_blocks( multiple + 1 /** one for meta info **/,
                                                                      &data->buffer->heap ); 
                                                               
        std::cout << "block start : "       << block_base   << "\n";
        std::cout << "blocks requested : "  << multiple     << "\n";
        //allocate base
        void *meta_alloc    = ipc::buffer::translate( (void *) &data->buffer->data,
                                                      block_base /** base offset **/ );
        assert( meta_alloc != nullptr );                                                      
        //make an actual meta-data structure
        auto *meta          = new (meta_alloc) ipc::allocate_metadata( 
            block_base /** help the allocator find the base and de-allocate this block **/,
            multiple   /** count of blocks allocated, not including this one **/
        );
        //set local allocation
        std::cout << "meta_blocks: " << meta_blocks << "\n";
        data->local_allocation = ipc::buffer::translate( (void *) &data->buffer->data,
                                                         block_base + meta_blocks /** base offset **/ );
        //FIXME <- come back here                                                            
        data->bytes_available  = (1 << ipc::buffer_size_pow_two) * (multiple);
        data->local_meta       = meta;
    }
    //if not the right size, go to sem_post and we end up here. 
    
    /** UNLOCK **/
    if( sem_post( sem ) != 0 )
    {
        std::perror( "Failed to post semaphore, exiting given we can't recover from this!" );
        //FIXME - need a global error here
        exit( EXIT_FAILURE );
    }
    return( output );
    
#if DEBUG
    // Atomically decrement number of threads in critical section
    std::atomic_fetch_sub(&data->buffer->critical_count, 1);
#endif
    return( output );


}
