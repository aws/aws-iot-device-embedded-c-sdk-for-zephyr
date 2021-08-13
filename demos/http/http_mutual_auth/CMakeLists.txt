# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required( VERSION 3.13.1 )

find_package( Zephyr HINTS $ENV{ZEPHYR_BASE} )
project( http_mutual_auth )

FILE( GLOB app_sources src/*.c )
target_sources( app PRIVATE ${app_sources} )

# For getting filepaths relative to this C-SDK repository.
get_filename_component( CSDK_BASE "${CMAKE_SOURCE_DIR}/../../.." ABSOLUTE )

# Include HTTP demo common.
include( ${CSDK_BASE}/demos/http/common/httpCommonFilePaths.cmake)

# Include HTTP library's source and header path variables.
include( ${CSDK_BASE}/libraries/standard/coreHTTP/httpFilePaths.cmake )

# Include backoffAlgorithm library file path configuration.
include( ${CSDK_BASE}/libraries/standard/backoffAlgorithm/backoffAlgorithmFilePaths.cmake )

# Include logging sources.
include( ${CSDK_BASE}/demos/logging-stack/logging.cmake )

#Include transport library implementations for Zephyr.
include( ${CSDK_BASE}/platform/zephyr/zephyrFilePaths.cmake )

#Include wifi connection function for ESP.
include( ${CSDK_BASE}/platform/espressif/espressifFilePaths.cmake )

# Include clock.
include( ${CSDK_BASE}/platform/zephyr/clock/clockFilePaths.cmake )

target_sources( app
    PRIVATE 
        ${HTTP_DEMO_COMMON_SOURCES}
        ${HTTP_SOURCES}
        ${HTTP_THIRD_PARTY_SOURCES}
        ${BACKOFF_ALGORITHM_SOURCES}
        ${CLOCK_SOURCES}
        ${SOCKETS_SOURCES}
        ${MBEDTLS_SOURCES}
        ${WIFI_SOURCES}
)

target_include_directories( app
    PUBLIC
        ${HTTP_DEMO_COMMON_INCLUDE_DIRS}
        ${HTTP_INCLUDE_PUBLIC_DIRS}
        ${BACKOFF_ALGORITHM_INCLUDE_PUBLIC_DIRS}
        ${CMAKE_CURRENT_LIST_DIR}
        ${LOGGING_INCLUDE_DIRS}
        ${CLOCK_INCLUDE_DIRS}
        ${COMMON_TRANSPORT_INCLUDE_PUBLIC_DIRS}
        ${WIFI_INCLUDE_DIRS}
)
