
    static ipc::tx_code pop( PARENTNODE *channel, 
                             LOCKFREE_NODE **receive_node, 
                             void *buffer_base )
    {
        *receive_node = nullptr;
        /**
         * get these atomically, might just need a fence, 
         * but...dies on all arch with -O2 and above. 
         */
        ipc::ptr_offset_t local_head_offset    = ipc::invalid_ptr_offset;
        ipc::ptr_offset_t local_tail_offset    = ipc::invalid_ptr_offset;



        //load up current head into a local copy
        local_head_offset = 
            channel->ctrl_all.data_head.load( std::memory_order_relaxed );

        //translate local head to an actual address
        auto *local_head_ptr = 
            (LOCKFREE_NODE*) TRANSLATE::translate_block( buffer_base, 
                                                         local_head_offset );
        auto local_next_offset = 
            local_head_ptr->next.load( std::memory_order_relaxed );
        
        local_tail_offset = 
            channel->ctrl_all.data_tail.load( std::memory_order_relaxed );


        
        if( local_head_offset == local_tail_offset )
        {
            if( local_head_ptr->_type == ipc::nodebase::dummy )
            {
                return( ipc::tx_retry );
            }
            else
            {
                //single node, non-dummy, let's add one so we can proceed
                auto *dummy_ptr = (LOCKFREE_NODE*) TRANSLATE::translate_block( buffer_base, 
                                                                               channel->meta.dummy_node_offset );
                self_t::push( channel, dummy_ptr, buffer_base );
                return( ipc::tx_retry );
            }
        }
        else if( local_next_offset == ipc::nodebase::init_offset() )
        {
            
            //head != tail, but still no local_next...
            //safe thing is to wait and come around again
            return( ipc::tx_retry );
        }
        else
        {
            /** swing our block value in as tail **/
            const bool
            success = 
             channel->ctrl_all.data_head.compare_exchange_weak( 
                         local_head_offset  /** expected ref **/,
                         local_next_offset  /** desired value **/,
                         std::memory_order_release /** memory order success **/,
                         std::memory_order_relaxed /** memory order failure **/ );
            if( success )
            {
                auto *success_local_head_ptr = 
                    (LOCKFREE_NODE*) TRANSLATE::translate_block( buffer_base, 
                                                                 local_head_offset );
                /**
                 * no need to go back and get the channel offset, if it's 
                 * the dummy, there's only one, push it. 
                 */
                if( success_local_head_ptr->_type == ipc::nodebase::dummy )
                {
                    self_t::push( channel, success_local_head_ptr, buffer_base );
                    return( ipc::tx_retry );
                }
                else
                {
                    *receive_node =  success_local_head_ptr;
                    return( ipc::tx_success );
                }
            }
            else
            {
                return( ipc::tx_retry );
            }
        }
        return( ipc::tx_retry );

    }
