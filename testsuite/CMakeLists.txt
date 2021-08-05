list( APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )
set( CMAKE_INCLUDE_CURRENT_DIR ON )

set( TESTAPPS #list apps
        genericnode
        zeronode
        bignode
        translateaddress
        locked_node_insert
        locked_node_remove
        locked_node_find
        lf_mn_node_insert
        lf_mn_node_remove
        lf_mn_node_pop
        lf_mn_node_insert_remove_twothreads
        lf_spsc_node_insert_remove
        lf_spsc_node_insert_remove_twothreads
        calculateOffset
        allocateBuffer
        allocationTest
        allocationMultiChannelOpen
        allocationMultiChannelNoChannel
        channelinfo_spacing
        #allocationMultithreaded
        #allocationMultiprocess
        #nodeCounter
    )


foreach( APP ${TESTAPPS} )
 add_executable( "${APP}_buffer" "${APP}.cpp" )
 target_link_libraries( "${APP}_buffer" buffer heapalloc pthread atomic rt ${CMAKE_NUMA_LIB} )
 add_test( NAME "${APP}_buffer_test" COMMAND "${APP}_buffer" )
endforeach( APP ${TESTAPPS} )
