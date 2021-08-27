project( HeapAlloc )
cmake_minimum_required(VERSION 3.10)
list( APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake )
include( CheckSTD )

add_subdirectory( src )
add_subdirectory( include )

##
# enable unit tests
##
if( NOT DEFINED ${BUILD_TESTS} )
mark_as_advanced( BUILD_TESTS )
set( BUILD_TESTS true CACHE BOOL "Tests build target available if true" )
endif( NOT DEFINED ${BUILD_TESTS} )

if( BUILD_TESTS )
##
# Set up unit tests
##
enable_testing()
add_subdirectory( testsuite )
endif( BUILD_TESTS )

