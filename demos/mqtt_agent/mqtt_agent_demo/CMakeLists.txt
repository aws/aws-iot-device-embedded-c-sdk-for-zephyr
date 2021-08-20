# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(mqtt_mutual_auth)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

#for getting filepaths relative to the demo, but it's better to do it relative to the zephyr base
get_filename_component(CSDK_BASE "${CMAKE_SOURCE_DIR}/../../.." ABSOLUTE)

# Include MQTT library's source and header path variables.
include( ${CSDK_BASE}/libraries/standard/coreMQTT/mqttFilePaths.cmake )

include( ${CSDK_BASE}/libraries/standard/coreMQTT-Agent/mqttAgentFilePaths.cmake)

# Include backoffAlgorithm library file path configuration.
include( ${CSDK_BASE}/libraries/standard/backoffAlgorithm/backoffAlgorithmFilePaths.cmake )

# Include logging sources.
include( ${CSDK_BASE}/demos/logging-stack/logging.cmake )

#Include transport library implementations for Zephyr.
include( ${CSDK_BASE}/platform/zephyr/zephyrFilePaths.cmake )

#Include wifi connection function for ESP.
include( ${CSDK_BASE}/platform/espressif/espressifFilePaths.cmake)

# Include clock.
include( ${CSDK_BASE}/platform/zephyr/clock/clockFilePaths.cmake)

target_sources(app
    PRIVATE 
        ${MQTT_SOURCES}
        ${MQTT_AGENT_SOURCES}
        ${MQTT_SERIALIZER_SOURCES}
        ${BACKOFF_ALGORITHM_SOURCES}
        ${CLOCK_SOURCES}
        ${SOCKETS_SOURCES}
        ${MBEDTLS_SOURCES}
        ${WIFI_SOURCES}
        ${MQTT_AGENT_ZEPHYR_SOURCES}
)

target_include_directories(app
    PUBLIC
        ${MQTT_INCLUDE_PUBLIC_DIRS}
        ${MQTT_AGENT_INCLUDE_PUBLIC_DIRS}
        ${BACKOFF_ALGORITHM_INCLUDE_PUBLIC_DIRS}
        ${CMAKE_CURRENT_LIST_DIR}
        ${LOGGING_INCLUDE_DIRS}
        ${CLOCK_INCLUDE_DIRS}
        ${COMMON_TRANSPORT_INCLUDE_PUBLIC_DIRS}
        ${WIFI_INCLUDE_DIRS}
        ${MQTT_AGENT_ZEPHYR_INCLUDE_PUBLIC_DIRS}
)
