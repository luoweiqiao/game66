
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
add_include_path( ${MYSQL_INCLUDE}  )

add_include_path( ../../../server_frame/include )
add_include_path( ../../../server_frame/include/luatinker )
add_include_path( ../../../tdr_auto/src )
add_include_path( ../src/ )
add_include_path( ../src/poker/ )
add_include_path( ../src/pb/ )

add_source_files( ../src/*.* )
add_source_files( ../src/poker/*.* )
add_source_files( ../src/pb/*.* )

add_link_path( ../../../server_frame/lib )
add_link_path( ../../../tdr_auto )
add_link_path( ${COMMON_LIBPATH} )
add_link_path( ${MYSQL_LIBPATH} )
	
	
build_static_lib( common ../../../server_frame/lib )
