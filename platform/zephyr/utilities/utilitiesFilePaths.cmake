# This file is to add source files and include directories
# into variables so that it can be reused from different repositories
# in their Cmake based build system by including this file.
#
# Files specific to the repository such as test runner, platform tests
# are not added to the variables.

# Backoff Algorithm library source files.
set( UTILITIES_SOURCES
     "${CMAKE_CURRENT_LIST_DIR}/source/mbedtls_error.c" )

# Backoff Algorithm library Public Include directories.
set( UTILITIES_INCLUDE_DIRS
     "${CMAKE_CURRENT_LIST_DIR}/source/include" )
