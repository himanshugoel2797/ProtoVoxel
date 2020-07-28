cmake_minimum_required(VERSION 3.0.0)
project(ProtoVoxel VERSION 0.1.0)


include(CTest)
enable_testing()

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

add_library(glad ${CMAKE_SOURCE_DIR}/glad/src/glad.c)
target_include_directories(glad PRIVATE ${CMAKE_SOURCE_DIR}/glad/include)

file(GLOB_RECURSE SRC_FILES ${CMAKE_SOURCE_DIR}/src/*)
add_executable(ProtoVoxel main.cpp ${SRC_FILES})
set_property(TARGET ProtoVoxel PROPERTY CXX_STANDARD 17)
target_link_libraries(ProtoVoxel glfw glad OpenGL::GL ${CMAKE_DL_LIBS})
target_include_directories(ProtoVoxel PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/glad/include ${CMAKE_SOURCE_DIR})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)