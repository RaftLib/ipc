cmake_minimum_required(VERSION 3.4 FATAL_ERROR)
project(shm)
set( version 1.1 )
set( CMAKE_INCLUDE_CURRENT_DIR ON )



##
# at this point we just need a warning
##
if( CMAKE_HOST_UNIX )
find_library( NUMA_LIBRARY
              NAMES numa
              PATHS
              ${CMAKE_LIBRARY_PATH}
              $ENV{NUMA_PATH}/lib
              /usr/lib
              /usr/local/lib
              /opt/local/lib )

find_path(  NUMA_INCLUDE_DIRS
            NAME numaif.h
            PATHS
              ${CMAKE_LIBRARY_PATH}$
              $ENV{NUMA_PATH}/lib$
              /usr/lib$
              /usr/local/lib$
              /opt/local/lib )

if( NUMA_LIBRARY AND NUMA_INCLUDE_DIRS )
    set( CMAKE_NUMA_LIBS ${NUMA_LIBRARY} )
    add_definitions( "-DPLATFORM_HAS_NUMA=1" )
    include_directories( ${NUMA_INCLUDE_DIRS} )
else( NUMA_LIBRARY AND NUMA_INCLUDE_DIRS )
##
# get machine type
##
    execute_process( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )
    execute_process( COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/findnodes.pl
                     COMMAND tr -d '\n' 
                     OUTPUT_VARIABLE HASNUMA )
    if( HASNUMA EQUAL 0 )
        ## no NUMA
        message( STATUS "no NUMA needed" )
        add_definitions( "-DPLATFORM_HAS_NUMA=0" )
    else( HASNUMA EQUAL 0 )
        ## needs NUMA but we don't have it
        message( FATAL_ERROR "You are compiling on a NUMA system, you must install libnuma" )
    endif( HASNUMA EQUAL 0 )
endif( NUMA_LIBRARY AND NUMA_INCLUDE_DIRS )

endif( CMAKE_HOST_UNIX )


include_directories ( ${PROJECT_SOURCE_DIR} )
include_directories ( ${PROJECT_SOURCE_DIR}/include )

add_subdirectory( lib )

mark_as_advanced( BUILD_TESTS )
set( BUILD_TESTS trrue CACHE BOOL "Tests build target available if true" )
if( BUILD_TESTS )
    enable_testing()
    add_subdirectory( testsuite )
endif()
