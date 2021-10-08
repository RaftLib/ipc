list( APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )
set( CMAKE_INCLUDE_CURRENT_DIR ON )

set( TESTAPPS #list apps
        allocateBuffer
        allocationTest
        allocationMultiChannelOpen
        allocationMultiChannelNoChannel
        bignode
        calculateOffset
        channelinfo_spacing
        genericnode
        haschannels
        locked_node_insert
        locked_node_remove
        locked_node_find
        #lf_mn_node_insert
        #lf_mn_node_remove
        #lf_mn_node_pop
        #lf_mn_node_insert_remove_twothreads
        lf_spsc_node_insert_remove
        lf_spsc_node_insert_remove_twothreads
        multiChannelIteration
        record_size
        shared_seg_two_process
        shared_seg_two_process_has_channel
        spsc_two_threads
        spsc_two_processes
        spsc_two_processes_has_data
        spsc_two_processes_multi_channel
        translateaddress
        zeronode
        #allocationMultithreaded
        #allocationMultiprocess
        #nodeCounter
    )


foreach( APP ${TESTAPPS} )
 add_executable( "${APP}_buffer" "${APP}.cpp" )
 target_link_libraries( "${APP}_buffer" ipcbuffer heapalloc pthread ${CMAKE_RT_LIBS} ${CMAKE_NUMA_LIB} )
 add_test( NAME "${APP}_buffer_test" COMMAND "${APP}_buffer" )
endforeach( APP ${TESTAPPS} )
