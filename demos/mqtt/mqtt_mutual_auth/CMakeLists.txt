# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(mqtt_mutual_auth)

message(STATUS "ZEPHYR BASE IS: ${ZEPHYR_BASE}")
message(STATUS "CMAKE SOURCE DIR IS: ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE CURRENT LIST DIR IS: ${CMAKE_CURRENT_LIST_DIR}")

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

#for getting filepaths relative to the demo, but it's better to do it relative to the zephyr base
#get_filename_component(DIR_ONE_ABOVE ../ ABSOLUTE)
get_filename_component(CSDK_BASE "${ZEPHYR_BASE}/../aws-iot-device-sdk-embedded-C" ABSOLUTE)
message(STATUS "CSDK BASE IS: ${CSDK_BASE}")

# Include MQTT library's source and header path variables.
#include( ${ZEPHYR_BASE}/samples/boards/esp32/coreMQTT/mqttFilePaths.cmake)
include( ${CSDK_BASE}/libraries/standard/coreMQTT/mqttFilePaths.cmake )

# Include backoffAlgorithm library file path configuration.
#include( ${ZEPHYR_BASE}/samples/boards/esp32/backoffAlgorithm/backoffAlgorithmFilePaths.cmake )
include( ${CSDK_BASE}/libraries/standard/backoffAlgorithm/backoffAlgorithmFilePaths.cmake )

# Include logging sources.
#include( ${ZEPHYR_BASE}/samples/boards/esp32/logging-stack/logging.cmake )
include( ${CSDK_BASE}/demos/logging-stack/logging.cmake )

# Include utilities.
include( ${CSDK_BASE}/platform/zephyr/utilities/utilitiesFilePaths.cmake)

include( ${CSDK_BASE}/platform/zephyr/zephyrFilePaths.cmake )

target_sources(app
    PRIVATE 
        ${MQTT_SOURCES}
        ${MQTT_SERIALIZER_SOURCES}
        ${BACKOFF_ALGORITHM_SOURCES}
        ${UTILITIES_SOURCES}
        ${SOCKETS_SOURCES}
        ${MBEDTLS_SOURCES}
)

target_include_directories(app
    PUBLIC
    ${MQTT_INCLUDE_PUBLIC_DIRS}
        ${BACKOFF_ALGORITHM_INCLUDE_PUBLIC_DIRS}
        ${CMAKE_CURRENT_LIST_DIR}
        ${LOGGING_INCLUDE_DIRS}
        ${UTILITIES_INCLUDE_DIRS}
        ${COMMON_TRANSPORT_INCLUDE_PUBLIC_DIRS}
)