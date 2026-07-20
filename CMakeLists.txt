cmake_minimum_required(VERSION 3.16)
project(slabel VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets Test LinguistTools)

file(GLOB SLABEL_SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_library(slabel SHARED ${SLABEL_SOURCES})
target_compile_definitions(slabel PRIVATE SLABEL_BUILD)
target_include_directories(slabel PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(slabel PUBLIC Qt5::Widgets)

enable_testing()
add_subdirectory(tests)
add_subdirectory(examples)
