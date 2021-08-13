set( CMAKE_INCLUDE_CURRENT_DIR ON )

find_package( LIBRT )
add_library( buffer 
                sem.cpp
                buffer.cpp 
                translate.cpp
                genericnode.cpp
                traceclientexception.cpp 
                allocationexception.cpp 
                buffer_base.cpp )


target_link_libraries( buffer pthread shm heapalloc ${CMAKE_RT_LIBS}  ${CMAKE_NUMA_LIBS} )


install( TARGETS buffer
         ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib )
