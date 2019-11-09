# cmake build list
# Copyright (C) 2013 toney
# @author: toney
# 2013-04-18
# landServer cmake

cmake_minimum_required( VERSION 2.8.3 )

include( ../../../cmake_config/config.cmake )
include( ../../../cmake_config/macros.cmake )
include( ../../../cmake_config/common_path.cmake )


clear_all_list()

add_include_path( ../../../server_frame/include )
add_include_path( ../../common/src)
add_include_path( ../../common/src/pb)
add_include_path( ${COMMON_INCLUDE} )
add_include_path( ${MYSQL_INCLUDE}  )
add_include_path( ../../../server_frame/include/luatinker )
add_include_path( ../../../server_frame/include/network )

add_include_path( ../../game_server/src )
add_include_path( ../src )

add_source_files( ../src/*.* )

add_link_path( ../../../server_frame/lib )

add_link_path( ${COMMON_LIBPATH} )
add_link_path( ${MYSQL_LIBPATH} )

add_link_lib( mysqlclient )
add_link_lib( pthread )
add_link_lib( dl )
add_link_lib( rt )
add_link_lib( gamesvr )
add_link_lib( common  )
add_link_lib( svrlib.a )
add_link_lib( iconv )
#add_link_lib( curl )
add_link_lib( protobuf )
add_link_lib( hiredis )	
add_link_lib( net )
	
build_executable( majiangServer ../../../release/majiang_server/ )
