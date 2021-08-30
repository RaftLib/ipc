list( APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )


set( IPCINCDIR ${CMAKE_INSTALL_PREFIX}/include )

##
# create raftinc dir in includes
##
install( DIRECTORY DESTINATION ${IPCINCDIR} )

file( GLOB_RECURSE TEMPLATES    *.tcc   )
file( GLOB_RECURSE CPP_HEADERS  *.hpp   )
file( GLOB_RECURSE C_HEADERS    *.h     )

set( INST_INC ${TEMPLATES} ${CPP_HEADERS} ${C_HEADERS} buffer )

foreach( INST_FILE ${INST_INC} )
    install( FILES ${INST_FILE} DESTINATION ${IPCINCDIR} )
endforeach( INST_FILE ${INST_INC} )
