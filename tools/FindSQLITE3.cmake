# Try to find the SQLite3 lib
# Once done this will define:
#
#  SQLITE3_FOUND - system has SQLite3
#  SQLITE3_INCLUDE_DIRS - SQLite3 include directory
#  SQLITE3_LIBRARIES - libraries needed to use SQLite3
#
# Copyright (c) 2018 Witold Piłat <witold.pilat@gmail.com>
#
# Redistribution and use is allowed according to the terms of the GPL2 license.
# For details see the accompanying LICENSE file.

find_path(SQLITE3_INCLUDE_DIR NAMES sqlite3.h)
find_library(SQLITE3_LIBRARY NAMES sqlite3)

# Handle QUIET and REQUIRED; set SQLITE3_FOUND
find_package_handle_standard_args(SQLITE3 DEFAULT_MSG SQLITE3_LIBRARY SQLITE3_INCLUDE_DIR)

if(SQLITE3_FOUND)
	set(SQLITE3_INCLUDE_DIRS ${SQLITE3_INCLUDE_DIR})
	set(SQLITE3_LIBRARIES ${SQLITE3_LIBRARY})
else()
	set(SQLITE3_INCLUDE_DIRS)
	set(SQLITE3_LIBRARIES)
endif()

mark_as_advanced(SQLITE3_INCLUDE_DIRS SQLITE3_LIBRARIES)
