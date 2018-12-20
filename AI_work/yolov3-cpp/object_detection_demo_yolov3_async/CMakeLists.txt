cmake_minimum_required(VERSION 2.8)

set(TARGET_NAME "object_detection_demo_yolov3_async")

if( BUILD_DEMO_NAME AND NOT ${BUILD_DEMO_NAME} STREQUAL ${TARGET_NAME} )
    message(STATUS "DEMO ${TARGET_NAME} SKIPPED")
    return()
endif()

find_package(OpenCV)
if(NOT(OpenCV_FOUND))
    message(STATUS "OPENCV is disabled or not found, " ${TARGET_NAME} " skiped")
    return()
endif()

file (GLOB MAIN_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        )

file (GLOB MAIN_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
        )

# Create named folders for the sources within the .vcproj
# Empty name lists them directly under the .vcproj
source_group("src" FILES ${MAIN_SRC})
source_group("include" FILES ${MAIN_HEADERS})

include_directories (${OpenCV_INCLUDE_DIRS})
link_directories(${LIB_FOLDER})

# Create library file from sources.
set(DSOURCES mqtt.cpp)
add_executable(${TARGET_NAME} ${MAIN_SRC} ${MAIN_HEADERS} ${DSOURCES})

add_dependencies(${TARGET_NAME} gflags pahomqtt)

set_target_properties(${TARGET_NAME} PROPERTIES "CMAKE_CXX_FLAGS" "${CMAKE_CXX_FLAGS} -fPIE"
        COMPILE_PDB_NAME ${TARGET_NAME})


target_link_libraries(${TARGET_NAME} IE::ie_cpu_extension ${InferenceEngine_LIBRARIES} gflags ${OpenCV_LIBRARIES} paho-mqtt3cs)

if(UNIX)
    target_link_libraries( ${TARGET_NAME} ${LIB_DL} pthread)
endif()

# 安装
install(TARGETS ${DETECTOR} DESTINATION bin)


