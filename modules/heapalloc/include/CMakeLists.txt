set( CMAKE_INCLUDE_CURRENT_DIR ON )


target_include_directories( heapalloc
                            PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ) 

message( STATUS "Including directory ${CMAKE_CURRENT_SOURCE_DIR}" )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
