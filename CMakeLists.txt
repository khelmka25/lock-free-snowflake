cmake_minimum_required(VERSION 3.20)
project(a CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(${PROJECT_NAME} main.cc)

file(GLOB_RECURSE SOURCE_DIR src/*.cc)
target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_DIR})

target_include_directories(${PROJECT_NAME} 
    PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/deps/argh
)

target_compile_options(${PROJECT_NAME} 
    PRIVATE 
    -Wconversion 
    -Wall 
    -Wextra
    -Wpedantic
    -O3
)