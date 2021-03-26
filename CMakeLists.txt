  
cmake_minimum_required(VERSION 3.2)
project(coreaudio-watcher)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

set(SOURCE_FILES src/main.cpp)

set(COREAUDIO_WATCHER_INSTALL_BIN_DIR ${PROJECT_SOURCE_DIR}/bin)

find_library(COREAUDIO CoreAudio)
if (NOT COREAUDIO)
    message(FATAL_ERROR "CoreAudio not found")
endif()

find_library(COREFOUNDATION CoreFoundation)
if (NOT COREFOUNDATION)
    message(FATAL_ERROR "CoreFoundation not found")
endif()

add_executable(coreaudio-watcher ${SOURCE_FILES})
target_link_libraries(coreaudio-watcher ${COREAUDIO} ${COREFOUNDATION})
install(TARGETS coreaudio-watcher DESTINATION ${COREAUDIO_WATCHER_INSTALL_BIN_DIR})