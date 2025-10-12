cmake_minimum_required(VERSION 3.20)
project(a CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

add_executable(${PROJECT_NAME} main.cc)

target_compile_options(${PROJECT_NAME} 
    PRIVATE 
    -Wconversion 
    -Wall 
    -Wextra
    -Wpedantic
)

aux_source_directory(src SOURCE_DIR)
target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_DIR})
target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)