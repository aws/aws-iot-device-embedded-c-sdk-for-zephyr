# This file is to add source files and include directories
# into variables so that it can be reused from different repositories
# in their Cmake based build system by including this file.
#
# Files specific to the repository such as test runner, platform tests
# are not added to the variables.

# Platform Clock library source files.
set( CLOCK_SOURCES
     "${CMAKE_CURRENT_LIST_DIR}/source/clock.c" )

# Platform Clock library Include directories.
set( CLOCK_INCLUDE_DIRS
     "${CMAKE_CURRENT_LIST_DIR}/include" )
