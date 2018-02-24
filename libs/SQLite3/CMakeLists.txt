# Build our bundled SQlite3.
#
# It is built as a static relocatable library, and linked into any target that
# requires it.  Other targets in JK2MV that consume need to add
# SQLITE3_INCLUDE_DIRS and SQLITE3_LIBRARIES to their lists of include
# directories and target link libraries.
#
# The bundled copy is produced by taking the files sqlite.{c,h}
# from the SQlite3 amalgamation zip.

# These settings only apply to this directory.
include_directories(.)

if(MSVC)
	# add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

add_definitions(-DSQLITE_THREADSAFE=0)
add_definitions(-DSQLITE_OMIT_DEPRECATED)
add_definitions(-DSQLITE_OMIT_LOAD_EXTENSION)

add_library(sqlite3 STATIC sqlite3.c)

if(NOT WIN32)
	# required only for thread-safety
	# find_package(Threads)
	# target_link_libraries(sqlite3 m ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})
endif()

# Let consumers get at our bundled copy in the standard CMake way.  These
# variables are not set in the cache, but instead shadow the variables in the
# cache.
set(SQLITE3_LIBRARIES sqlite3 PARENT_SCOPE)
set(SQLITE3_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/libs/SQlite3 PARENT_SCOPE)
set(SQLITE3_FOUND TRUE)

mark_as_advanced(SQLITE3_LIBRARIES SQLITE3_INCLUDE_DIRS)

# We need to build it as position-independent code, because it might get linked
# into dynamic libraries.
set_property(TARGET sqlite3 PROPERTY POSITION_INDEPENDENT_CODE True)
set_property(TARGET sqlite3 PROPERTY FOLDER "libs")
