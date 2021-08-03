# This file is to add source files and include directories
# into variables so that it can be reused from different repositories
# in their Cmake based build system by including this file.
#
# Files specific to the repository such as test runner, platform tests
# are not added to the variables.

# HTTP common source files.
set( HTTP_DEMO_COMMON_SOURCES
     "${CMAKE_CURRENT_LIST_DIR}/src/http_demo_utils.c" )

# HTTP common include directories.
set( HTTP_DEMO_COMMON_INCLUDE_DIRS
     "${CMAKE_CURRENT_LIST_DIR}/include" )
     