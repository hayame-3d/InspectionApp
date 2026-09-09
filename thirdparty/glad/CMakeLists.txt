cmake_minimum_required(VERSION 3.20)
project(glad LANGUAGES C)

# glad2 web generator produces gl.h instead of glad.h
# It may produce src/gl.c or src/glad.c depending on version
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/gl.c)
    add_library(glad STATIC src/gl.c)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/glad.c)
    add_library(glad STATIC src/glad.c)
else()
    message(FATAL_ERROR "GLAD source file not found. Expected src/gl.c or src/glad.c")
endif()

target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
