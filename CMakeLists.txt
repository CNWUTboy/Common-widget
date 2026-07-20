cmake_minimum_required(VERSION 3.16)
project(slabel VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets Test LinguistTools)

file(GLOB SLABEL_SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB SLABEL_HEADERS CONFIGURE_DEPENDS include/slabel/*.h)

# 头文件也纳入目标源码列表，确保 AUTOMOC 能发现跨目录（include/ vs src/）
# 声明 Q_OBJECT 的头文件（如 SControlCore.h）并为其生成 moc
add_library(slabel SHARED ${SLABEL_SOURCES} ${SLABEL_HEADERS})
target_compile_definitions(slabel PRIVATE SLABEL_BUILD)
target_include_directories(slabel PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(slabel PUBLIC Qt5::Widgets)

enable_testing()
add_subdirectory(tests)
add_subdirectory(examples)
