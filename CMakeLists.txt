cmake_minimum_required(VERSION 3.16)
project(slabel VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets Test LinguistTools)

# 生成 .qm
qt5_add_translation(SLABEL_QM
    translations/slabel_zh_CN.ts
    translations/slabel_en.ts)
add_custom_target(slabel_translations ALL DEPENDS ${SLABEL_QM})

# 拷贝主题与 qm 到构建目录，供测试与示例运行时读取
file(COPY themes DESTINATION ${CMAKE_BINARY_DIR})
add_custom_command(TARGET slabel_translations POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/translations
    COMMAND ${CMAKE_COMMAND} -E copy ${SLABEL_QM} ${CMAKE_BINARY_DIR}/translations)

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
