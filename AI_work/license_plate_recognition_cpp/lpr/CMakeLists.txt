cmake_minimum_required(VERSION 3.5.1)
project(license_plate_recognition)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
find_package(OpenCV 3.3.0 REQUIRED)
include_directories( ${OpenCV_INCLUDE_DIRS})
include_directories(
        /usr/include/freetype2/
        include)

set(SRC_DETECTION src/PlateDetection.cpp  src/util.h include/PlateDetection.h)

set(SRC_FINEMAPPING src/FineMapping.cpp )

set(SRC_FASTDESKEW src/FastDeskew.cpp )

set(SRC_SEGMENTATION src/PlateSegmentation.cpp )

set(SRC_RECOGNIZE src/Recognizer.cpp src/CNNRecognizer.cpp)

set(SRC_PIPLINE src/Pipeline.cpp)

set(SRC_SEGMENTATIONFREE src/SegmentationFreeRecognizer.cpp )

set(SOURCE_FILES src/main.cpp src/CvxText.cpp)
add_executable(license_plate_recognition ${SOURCE_FILES} ${SRC_DETECTION}  ${SRC_FINEMAPPING}  ${SRC_FASTDESKEW}  ${SRC_SEGMENTATION}  ${SRC_RECOGNIZE}  ${SRC_PIPLINE} ${SRC_SEGMENTATIONFREE})
target_link_libraries(license_plate_recognition ${OpenCV_LIBS} freetype)
