set( CMAKE_INCLUDE_CURRENT_DIR ON )

##
# source files
##
add_library( heapalloc STATIC
                _512bits.cpp
           )
# Enable warnings if using clang or gcc.
if ( "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" 
  OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" )

target_compile_options( heapalloc
                        PUBLIC
                        "-W" "-Wall" "-Wextra" "-Wpedantic" )

endif()

install( TARGETS heapalloc
         ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib )
