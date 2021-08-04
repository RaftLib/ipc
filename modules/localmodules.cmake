##
# list local dependencies here
##
set( LOCAL_MODULES heapalloc )


foreach( M ${LOCAL_MODULES} )
    message( STATUS "adding local submodule \"${M}\"" )
    add_subdirectory( ${CMAKE_CURRENT_SOURCE_DIR}/modules/${M} )
    include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/modules/${M}/include )
endforeach()
 
