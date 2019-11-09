
# cmake build list
# Copyright (C) 2013 toney
# @author: toney
# 2013-05-7
# common cmake
cmake_minimum_required( VERSION 2.8.3 )

include( ../../../cmake_config/config.cmake )
include( ../../../cmake_config/macros.cmake )
include( ../../../cmake_config/common_path.cmake )

clear_all_list()

add_include_path( ${COMMON_INCLUDE} )

add_source_files( ../src/*.* )

add_definitions( --no-warnings )

add_link_path( ../../../server_frame/lib )
	
build_static_lib( landRobotAiEx ../../../server_frame/lib )
