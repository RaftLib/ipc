cmake_minimum_required(VERSION 3.4 FATAL_ERROR)
project(buffer)
set( version 1.1 )
set( CMAKE_INCLUDE_CURRENT_DIR ON )
    
set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_C_STANDARD 99 )
set( CXX_STANDARD ON )
set( C_STANDARD ON )

list( APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake )

include(CheckGitDep)

include(CheckCXXCompilerFlag)

CHECK_CXX_COMPILER_FLAG("-faligned-new" HAS_ALIGNED_NEW)
if (HAS_ALIGNED_NEW)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -faligned-new")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -faligned-new")
endif()


# Set a variable so that we can detect debug mode in cpp files
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")



set( HELPER_DIR "${CMAKE_SOURCE_DIR}/helpers" )
##
# helper exec to get the L1 cache size from Linux/OS X/Win
##
execute_process( COMMAND ${CMAKE_CXX_COMPILER} ${HELPER_DIR}/cacheinfo.cpp -o ${HELPER_DIR}/cacheinfo)
execute_process( COMMAND ${HELPER_DIR}/cacheinfo 
                 COMMAND tr -d '\n' 
                 OUTPUT_VARIABLE L1D_LINE_SIZE )
execute_process( COMMAND rm -f ${HELPER_DIR}/cacheinfo )                 

if( NOT L1D_LINE_SIZE )
    set( L1D_LINE_SIZE 64 )
    message( INFO " Couldn't detect cache line size, set to: ${L1D_LINE_SIZE}" )
else()
    message( INFO " Detected cache line size, set to: ${L1D_LINE_SIZE}" )
endif()

##
# for cache line size
##
add_definitions( "-DL1D_CACHE_LINE_SIZE=${L1D_LINE_SIZE}" ) 

include( ${CMAKE_CURRENT_SOURCE_DIR}/modules/localmodules.cmake )

include_directories ( ${PROJECT_SOURCE_DIR} )
include_directories ( ${PROJECT_SOURCE_DIR}/include )
add_subdirectory( src )

mark_as_advanced( BUILD_TESTS )
set( BUILD_TESTS true CACHE BOOL "Tests build target available if true" )
if( BUILD_TESTS )
##
# Set up unit tests
##
enable_testing()
add_subdirectory( testsuite )
endif( BUILD_TESTS )
