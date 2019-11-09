rem  复制配置文件到server目录下

rem  清除旧有文件
del /Q ..\server\tdr_auto\src\*.h
del /Q ..\server\tdr_auto\src\*.cpp


rem 复制新数据

xcopy cs_msg\cpp\*.* ..\server\servers\common\src\pb  /Y /E

pause