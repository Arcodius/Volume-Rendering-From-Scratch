cmake_minimum_required(VERSION 3.20)

project(VR LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 20)

file(GLOB SRC_FILES CONFIGURE_DEPENDS src/*.cpp src/*.cu)

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE external/glm)

add_executable(VR ${SRC_FILES})

target_link_libraries(VR PRIVATE glm)