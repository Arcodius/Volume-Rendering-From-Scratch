cmake_minimum_required(VERSION 3.20)

project(VR LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

file(GLOB SRC_FILES CONFIGURE_DEPENDS src/*.cpp)

add_library(glm INTERFACE)
# Add your own glm directory
target_include_directories(glm INTERFACE C:/Users/Tiracolate/library/glm)

add_executable(VR ${SRC_FILES})

target_link_libraries(VR PRIVATE glm)