# cmake build config
# Copyright (C) 2013 toney
# @author: toney
# 2013-04-18

cmake_minimum_required( VERSION 2.8.3 )

set( INCLUDE_LIST "" )
set( SOURCE_LIST "" )
set( LINK_LIST "" )
set( LINK_LIBS "" )

set( CMAKE_COLOR_MAKEFILE TRUE )
set( CMAKE_CXX_COMPILER "g++" )

#set( WITH_WARNINGS 1)
set( LESS_WARNINGS 1)
#set( WITH_COREDEBUG 1)

#set( CMAKE_BUILD_TYPE "Debug" )
#set( CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb" )
set( CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -O0 -g -ggdb -static-libstdc++ -static-libgcc" )
#set( CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -O2" )

#set( CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -O3 -Wall" )

# setting build macro
# add_definitions( -D_BUILD_DIRECTIVE = '"${CMAKE_BUILD_TYPE}"' )
add_definitions( -fno-delete-null-pointer-checks )

# package overloads - Linux
if ( CMAKE_SYSTEM_NAME MATCHES "Linux" )
	set( JEMALLOC_LIBRARY "jemalloc" )
endif()

# set default configuration directory
if ( NOT CONF_DIR )
	set( CONF_DIR ${CMAKE_INSTALL_PREFIX}/etc )
	#message( STATUS "UNIX: Using default configuration directory" )
endif()

# setting default library directory
if ( NOT LIBSDIR )
	set( LIBSDIR ${CMAKE_INSTALL_PREFIX}/lib )
	#message( STATUS "UNIX: Using default library directory" )
endif()

if ( USE_SFMT )
	if ( PLATFORM EQUAL 32 )
		# required on 32-bit systems to enable SSE2 ( standard on x64 )
		add_definitions( -msse2 -mfpmath=sse )
	endif ()
	add_definitions( -DHAVE_SSE2 -D__SSE2__ )
	#message( STATUS "GCC: SFMT enabled, SSE2 flags forced" )
endif ()

if ( WITH_WARNINGS )
	add_definitions( -Wall -Wfatal-errors -Wextra )
	#message( STATUS "GCC: All warnings enabled" )
elseif(LESS_WARNINGS)
	add_definitions( -Wall -Wno-unused -Wfatal-errors -Wextra -Wno-deprecated -Wno-non-virtual-dtor -fno-strict-aliasing)
	#-Wshadow )
	#message( STATUS "GCC: less warnings enabled" )
else()
	add_definitions( --no-warnings )
	#message( STATUS "GCC: All warnings disabled" )
endif ()

if ( WITH_COREDEBUG )
	add_definitions( -ggdb3 )
	#message( STATUS "GCC: Debug-flags set -ggdb3" )
endif ()
