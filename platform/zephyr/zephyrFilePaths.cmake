set( SOCKETS_SOURCES
     ${CMAKE_CURRENT_LIST_DIR}/transport/src/sockets_zephyr.c )

set( PLAINTEXT_SOURCES
     ${CMAKE_CURRENT_LIST_DIR}/transport/src/plaintext_zephyr.c )

set( MBEDTLS_SOURCES
     ${CMAKE_CURRENT_LIST_DIR}/transport/src/mbedtls_zephyr.c )

set( COMMON_TRANSPORT_INCLUDE_PUBLIC_DIRS
     ${CMAKE_CURRENT_LIST_DIR}/transport/include )