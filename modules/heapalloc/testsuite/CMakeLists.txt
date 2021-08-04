list( APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )
set( CMAKE_INCLUDE_CURRENT_DIR ON )

set( TESTAPPS #list apps
        contiguouszeros
        getnreturnblocks
        getreturnall
        leftright
        parent
        setbit
        unsetbit
        usage
    )


foreach( APP ${TESTAPPS} )
 add_executable( ${APP} "${APP}.cpp" )
 target_link_libraries( ${APP} heapalloc )
 target_include_directories( ${APP} PUBLIC ${CMAKE_SOURCE_DIR}/include )
 add_test( NAME "${APP}_test" COMMAND ${APP} )
endforeach( APP ${TESTAPPS} )
