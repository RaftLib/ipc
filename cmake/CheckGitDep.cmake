##
# check out other repos we need
##
set( DEPDIR git-dep )
set( DEPSRC ${PROJECT_SOURCE_DIR}/${DEPDIR} )
set( DEPBIN ${PROJECT_BINARY_DIR}/${DEPDIR} )
##
# add definitions needed bymodules, if any
##

##
# LIST MODULES HERE
##
include( ${DEPSRC}/gitmodules.cmake )

##
# NOW CHECK THEM OUT 
##
include(ExternalProject)

foreach( GMOD ${GIT_MODULES} )
 message( STATUS  "Initializing sub-module ${DEPSRC}/${GMOD} from git repo!" )
 execute_process( COMMAND git submodule init ${DEPSRC}/${GMOD}
                  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}) 
 execute_process( COMMAND git submodule update ${DEPSRC}/${GMOD} 
                  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
 ##
 # build execs if needed
 ##
 if( EXISTS ${DEPSRC}/${GMOD}/CMakeLists.txt )
    add_subdirectory( ${DEPSRC}/${GMOD} )
 endif( EXISTS ${DEPSRC}/${GMOD}/CMakeLists.txt )
 ##
 # assume we have an include dir in the sub-module
 ##
 if( EXISTS ${DEPSRC}/${GMOD}/include )
    include_directories( ${DEPSRC}/${GMOD}/include )
 endif( EXISTS ${DEPSRC}/${GMOD}/include )
 
 ##
 # assume that submodules could have processed headers
 # in the build tree include dir
 ##
 if( EXISTS ${DEPBIN}/${GMOD}/include )
    include_directories( ${DEPBIN}/${GMOD}/include )
 endif( EXISTS ${DEPBIN}/${GMOD}/include )
 ##
 # assume they could be in the build tree root dir
 ##
 if( EXISTS ${DEPBIN}/${GMOD}/ )
    include_directories( ${DEPBIN}/${GMOD}/ )
 endif( EXISTS ${DEPBIN}/${GMOD}/ )


endforeach( GMOD ${GIT_MODULES} )
