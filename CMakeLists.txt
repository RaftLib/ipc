cmake_minimum_required(VERSION 3.4 FATAL_ERROR)
project(ipcbuffer)
set( version 1.1 )
set( CMAKE_INCLUDE_CURRENT_DIR ON )
    
set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_C_STANDARD 99 )
set( CXX_STANDARD ON )
set( C_STANDARD ON )

list( APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake )

## for shm module
set( CPP_EXCEPTIONS 0 )
add_definitions( "-DUSE_CPP_EXCEPTIONS=0" )

include(CheckGitDep)

include(CheckCXXCompilerFlag)
include(ProjectInclude)

CHECK_CXX_COMPILER_FLAG("-faligned-new" HAS_ALIGNED_NEW)
if (HAS_ALIGNED_NEW)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -faligned-new")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -faligned-new")
endif()
    
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -mtune=native")


# Set a variable so that we can detect debug mode in cpp files
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")

subproject_enable_includes()
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
if( APPLE )
set( USE_SYSTEMV_SEM_DEFAULT true )
else( APPLE )
set( USE_SYSTEMV_SEM_DEFAULT false )
endif( APPLE )

set( USE_SYSTEMV_SEM ${USE_SYSTEMV_SEM_DEFAULT} CACHE BOOL "Enable use of SystemV semaphore behavior vs. POSIX semaphores." )
if( USE_SYSTEMV_SEM )
message( STATUS "Using SystemV semaphores." )
    set( SYSTEMV_SEM 1 )
    set( POSIX_SEM 0 )
else( USE_SYSTEMV_SEM )
##
# default is to use POSIX, unless APPLE
##
message( STATUS "Using POSIX semaphores." )
    set( SYSTEMV_SEM 0 )
    set( POSIX_SEM 1 )
endif( USE_SYSTEMV_SEM )

add_definitions( "-D_USE_SYSTEMV_SEM_=${SYSTEMV_SEM}" )
add_definitions( "-D_USE_POSIX_SEM_=${POSIX_SEM}" )

set( MODULEHEADER "ipc_moduleflags.hpp" )
configure_file( ${PROJECT_SOURCE_DIR}/include/ipc_moduleflags.hpp.in ${PROJECT_BINARY_DIR}/include/${MODULEHEADER} @ONLY )
install( FILES ${PROJECT_BINARY_DIR}/include/${MODULEHEADER} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/ )

include_directories ( ${PROJECT_SOURCE_DIR} )
include_directories ( ${PROJECT_SOURCE_DIR}/include )
include_directories ( ${PROJECT_BINARY_DIR}/include )

add_subdirectory( include )
add_subdirectory( src )

subproject_finalize_includes()

mark_as_advanced( BUILD_TESTS )
set( BUILD_TESTS true CACHE BOOL "Tests build target available if true" )
if( BUILD_TESTS )
##
# Set up unit tests
##
enable_testing()
add_subdirectory( testsuite )
endif( BUILD_TESTS )

##
# setup pkg-config files - for linux/unix/os x
##
if( NOT WIN32 )
##
# make and setup pkg-config
##
mark_as_advanced( PKG_CONFIG_PATHWAY )
set( PKG_CONFIG_PATH "" CACHE STRING "Set the pkg-config path, othwerwise will figure out" )
if( NOT PKG_CONFIG_PATH )
execute_process( COMMAND  pkg-config --variable pc_path pkg-config 
                 COMMAND  tr ':' '\n' 
                 COMMAND  head -n 1
                 OUTPUT_VARIABLE LOCAL_PKG_CONFIG_PATHWAY )
set( PKG_CONFIG_PATH ${LOCAL_PKG_CONFIG_PATHWAY} )
endif()

string(REPLACE ":" ";" PREFIX_LIST "${CMAKE_SYSTEM_PREFIX_PATH}")
list(FIND PREFIX_LIST ${CMAKE_INSTALL_PREFIX}  _index)

if(${_index} GREATER 0)
    file( MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/pkgconfig ) 
    set( PKG_CONFIG_PATH ${CMAKE_INSTALL_PREFIX}/pkgconfig )
    message( WARNING "You should set ${RED}PKG_CONFIG_PATH=${CMAKE_INSTALL_PREFIX}/pkgconfig/:$PKG_CONFIG_PATH ${ColourReset} when installing to non-standard prefix for pkg-config to work correctly!" )
else()
    message( STATUS "Setting PKG_CONFIG_PATH to: ${PKG_CONFIG_PATH}" )
endif()



configure_file( "ipc.pc.in" "ipc.pc" @ONLY )
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/ipc.pc DESTINATION ${CMAKE_INSTALL_PREFIX}/pkgconfig/ )

endif( NOT WIN32 )                     
