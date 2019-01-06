cmake_minimum_required (VERSION 2.8.11)
project(project)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/thirdparty")

if (CMAKE_BUILD_TYPE STREQUAL "")
    message(STATUS "CMAKE_BUILD_TYPE not defined, 'Release' will be used")
    set(CMAKE_BUILD_TYPE "Release")
endif()

if (NOT(BIN_FOLDER))
    if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        set (ARCH intel64)
    else()
        set (ARCH ia32)
    endif()

    set (BIN_FOLDER ${ARCH})
endif()

if (NOT(IE_MAIN_SOURCE_DIR))
    # in case if samples are built out of IE repo
    set (IE_MAIN_SAMPLES_DIR ${CMAKE_CURRENT_BINARY_DIR})
else()
    # in case if samples are built from IE repo
    set (IE_MAIN_SAMPLES_DIR ${IE_MAIN_SOURCE_DIR})
endif()

if(UNIX)   
    set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${IE_MAIN_SAMPLES_DIR}/${BIN_FOLDER}/${CMAKE_BUILD_TYPE}/lib)
    set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${IE_MAIN_SAMPLES_DIR}/${BIN_FOLDER}/${CMAKE_BUILD_TYPE}/lib)
    set (CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY ${IE_MAIN_SAMPLES_DIR}/${BIN_FOLDER}/${CMAKE_BUILD_TYPE})
    set (CMAKE_PDB_OUTPUT_DIRECTORY ${IE_MAIN_SAMPLES_DIR}/${BIN_FOLDER}/${CMAKE_BUILD_TYPE})
    set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${IE_MAIN_SAMPLES_DIR}/${BIN_FOLDER}/${CMAKE_BUILD_TYPE})
    set (LIBRARY_OUTPUT_DIRECTORY ${IE_MAIN_SAMPLES_DIR}/${BIN_FOLDER}/${CMAKE_BUILD_TYPE}/lib)
    set (LIBRARY_OUTPUT_PATH ${LIBRARY_OUTPUT_DIRECTORY}/lib)
endif()

find_package(InferenceEngine 1.4 REQUIRED)

if (UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Werror=return-type ")   
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wuninitialized -Winit-self -Wmaybe-uninitialized")
endif()

####################################
## to use C++11
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
####################################

set(GFLAGS_IS_SUBPROJECT TRUE)
add_subdirectory(thirdparty/gflags)


# Properties->C/C++->General->Additional Include Directories
include_directories (
    ${CMAKE_CURRENT_SOURCE_DIR}/common/format_reader
    ${InferenceEngine_INCLUDE_DIRS}
    ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/gflags/include
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

if (UNIX)
    SET(LIB_DL dl)
endif()

# Find OpenCV library if exists
find_package(OpenCV)
if(OpenCV_FOUND)
    include_directories(${OpenCV_INCLUDE_DIRS})
    add_definitions(-DUSE_OPENCV)
else()
    set (BUILD_VALIDATION_APP OFF)
    message(WARNING "No suitable OpenCV version detected, BUILD_VALIDATION_APP is set to OFF")
endif()

# 安装paho MQTT dependency
include(pahomqtt)

add_subdirectory(common/format_reader)

add_subdirectory(object_detection_demo_yolov3_async)

