project( tomahawk )

include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )
add_definitions( -DQT_PLUGIN )
add_definitions( -DQT_SHARED )
add_definitions( -DSIPDLLEXPORT_PRO )

set( jabberSources
    jabber.cpp
    jabber_p.cpp
)

set( jabberHeaders
    jabber.h
    jabber_p.h
)

include_directories( . ${CMAKE_CURRENT_BINARY_DIR} ..
   ${QT_INCLUDE_DIR}
   ${LIBJREEN_INCLUDE_DIR}
)

qt4_wrap_cpp( jabberMoc ${jabberHeaders} )
add_library( sip_jreen SHARED ${jabberSources} ${jabberMoc} )

IF( WIN32 )
SET( OS_SPECIFIC_LINK_LIBRARIES
    ${OS_SPECIFIC_LINK_LIBRARIES}
    "secur32.dll"
    "crypt32.dll"
    ${TOMAHAWK_LIBRARIES}
)
ENDIF( WIN32 )

target_link_libraries( sip_jreen
    ${QT_LIBRARIES}
    ${LIBJREEN_LIBRARY}
    ${OS_SPECIFIC_LINK_LIBRARIES}
)

IF( APPLE )
#  SET( CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-undefined dynamic_lookup" )
ENDIF( APPLE )

install( TARGETS sip_jreen DESTINATION lib )
