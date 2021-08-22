# This file is to add source files and include directories
# into variables so that it can be reused from different repositories
# in their Cmake based build system by including this file.
#
# Files specific to the repository such as test runner, platform tests
# are not added to the variables.

# Platform socket library source files.
set( SOCKETS_SOURCES
     ${CMAKE_CURRENT_LIST_DIR}/transport/src/sockets_zephyr.c )

# Platform plaintext library source files.
set( PLAINTEXT_SOURCES
     ${CMAKE_CURRENT_LIST_DIR}/transport/src/plaintext_zephyr.c )

# Platform mbedtls library source files.
set( MBEDTLS_SOURCES
     ${CMAKE_CURRENT_LIST_DIR}/transport/src/mbedtls_zephyr.c )

# Platform transport library include directories.
set( COMMON_TRANSPORT_INCLUDE_PUBLIC_DIRS
     ${CMAKE_CURRENT_LIST_DIR}/transport/include )

set( MQTT_AGENT_ZEPHYR_SOURCES
     ${CMAKE_CURRENT_LIST_DIR}/mqtt_agent/src/agent_interface_zephyr.c
     ${CMAKE_CURRENT_LIST_DIR}/mqtt_agent/src/subscription_manager.c )

set( MQTT_AGENT_ZEPHYR_INCLUDE_PUBLIC_DIRS
     ${CMAKE_CURRENT_LIST_DIR}/mqtt_agent/include )
