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
    
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -mtune=native")


# Set a variable so that we can detect debug mode in cpp files
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")


## 
# helpers
##
add_subdirectory(helpers)


##
# for cache line size
##
add_definitions( "-DL1D_CACHE_LINE_SIZE=${L1D_LINE_SIZE}" ) 

include( ${CMAKE_CURRENT_SOURCE_DIR}/modules/localmodules.cmake )

mark_as_advanced( USE_SYSTEMV_SEM )
set( USE_SYSTEMV_SEM false CACHE BOOL "Enable use of SystemV semaphore behavior vs. POSIX semaphores." )
if( USE_SYSTEMV_SEM )
message( STATUS "Using SystemV semaphores." )
    add_definitions( "-D_USE_SYSTEMV_SEM_=1" )
    add_definitions( "-D_USE_POSIX_SEM_=0" )
else( USE_SYSTEMV_SEM )
##
# default is to use POSIX
##
message( STATUS "Using POSIX semaphores." )
    add_definitions( "-D_USE_SYSTEMV_SEM_=0" )
    add_definitions( "-D_USE_POSIX_SEM_=1" )
endif( USE_SYSTEMV_SEM )

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
