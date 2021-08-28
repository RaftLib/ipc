##
# these two macros are identical, we need the first
# one to create the list (will happen no matter how
# the sub-projects are structured, then we need the
# second to append to the parent.
# usage is
# include( ProjectInclude )
# subproject_enable_includes()
# //rest of code goes here, including all sub-dir calls
# //this goes last
# subproject_finalize_includes()
##
macro( subproject_enable_includes )
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
    list( APPEND SUB_PROJECT_INC ${dir})
    list( REMOVE_DUPLICATES SUB_PROJECT_INC )
    set( SUB_PROJECT_INC ${SUB_PROJECT_INC} )
#    message( STATUS "->${dir}" )
endforeach()
endmacro()

macro( subproject_finalize_includes )
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
    list( APPEND SUB_PROJECT_INC ${dir})
    list( REMOVE_DUPLICATES SUB_PROJECT_INC )
    ##
    # solution for parent scope here -> https://stackoverflow.com/a/25217937
    ##
    get_directory_property( has_parent PARENT_DIRECTORY)
    if(has_parent)
        set( SUB_PROJECT_INC ${SUB_PROJECT_INC} PARENT_SCOPE )
    else()
        set( SUB_PROJECT_INC ${SUB_PROJECT_INC} )
    endif()
endforeach()
##
# auto include sub-project includes at each scope we run, 
# it will result in spurious includes, but, at least will 
# ensure includes are carried to each scope.
##
include_directories( ${SUB_PROJECT_INC} )
endmacro()

macro( subproject_print_includes )
get_property(dirs DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
    message(STATUS "including_directory -I'${dir}'")
endforeach()
endmacro()
