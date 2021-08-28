
list( APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake )
##
# list local dependencies here
##
set( LOCAL_MODULES heapalloc )
include( ProjectInclude )


foreach( M ${LOCAL_MODULES} )
    message( STATUS "adding local submodule \"${M}\"" )
    subproject_enable_includes()
    add_subdirectory( ${PROJECT_SOURCE_DIR}/modules/${M} )
    include_directories( ${PROJECT_SOURCE_DIR}/modules/${M}/include )
    subproject_finalize_includes()
endforeach()
 
