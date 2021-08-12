# This file is to add source files and include directories
# into variables so that it can be reused from different repositories
# in their Cmake based build system by including this file.
#
# Files specific to the repository such as test runner, platform tests
# are not added to the variables.

# Platform ESP library source files.
set( WIFI_SOURCES
     ${CMAKE_CURRENT_LIST_DIR}/wifi/src/esp_wifi_wrapper.c )

#Platform ESP library include directories.
set( WIFI_INCLUDE_DIRS
     ${CMAKE_CURRENT_LIST_DIR}/wifi/include )