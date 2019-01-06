cmake_minimum_required(VERSION 2.8.11)
project(FAC)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# 特殊色彩输入信息
string(ASCII 27 Esc)
set(CR "${Esc}[m")
set(Red "${Esc}[1;31m")
set(Green "${Esc}[1;32m")
set(Blue "${Esc}[1;34m")

# 如果需要，允许用户手动设置OpenCV路径
set(OpenCV_DIR CACHE PATH "Path to Intel OpenCV library")

message(STATUS "${Blue}Intel Face Access Control (FAC) Demo${CR}")

# 检查系统版本
message(STATUS "${Blue}Checking OS version...${CR}")

set(REQUIRED_OS_ID "Ubuntu")
set(REQUIRED_OS_VERSION "16.04")
include(osdetect)

# 检查是否安装OpenCV
message(STATUS "${Blue}Checking prerequisites...${CR}")

find_package(DemoOpenCV)
if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "${Red}Intel OpenCV was not found!\n${Blue}Have you sourced the ${Green}setupvars.sh${Blue} script provided by the Intel CV SDK?${CR}")
endif( )

include_directories(${OpenCV_INCLUDE_DIRS})

# 安装paho MQTT dependency
include(pahomqtt)

# 应用程序可执行文件
set(DETECTOR cvservice)
set(DSOURCES src/cvservice.cpp src/mqtt.cpp)
add_executable(${DETECTOR} ${DSOURCES})
add_dependencies(${DETECTOR} pahomqtt)
set_target_properties(${DETECTOR} ${TRAINER} PROPERTIES COMPILE_FLAGS "-pthread -std=c++11")
target_link_libraries (${DETECTOR} ${OpenCV_LIBS} paho-mqtt3cs)

# 安装
install(TARGETS ${DETECTOR} DESTINATION bin)

