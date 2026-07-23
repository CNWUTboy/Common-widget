cmake_minimum_required(VERSION 3.16)
project(slabel VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# 统一输出目录：所有 .dll 放在顶层，测试通过 PATH 查找
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets Test LinguistTools)

# ---- 显式列举源文件，避免 file(GLOB) 反模式 ----
set(SLABEL_SOURCES
    src/BindingEngine.cpp
    src/LanguageManager.cpp
    src/SBindable.cpp
    src/SControlBridge.cpp
    src/SControlCore.cpp
    src/SControlEngine.cpp
    src/slabel_version.cpp
    src/ThemeManager.cpp
)

set(SLABEL_HEADERS
    include/slabel/BindingEngine.h
    include/slabel/ISControl.h
    include/slabel/LanguageManager.h
    include/slabel/OperationState.h
    include/slabel/SBindable.h
    include/slabel/SControl.h
    include/slabel/SControlBridge.h
    include/slabel/SControlCore.h
    include/slabel/SControlEngine.h
    include/slabel/SGlobal.h
    include/slabel/ThemeManager.h
)

# ---- 翻译：.ts → .qm（构建时生成）----
qt5_add_translation(SLABEL_QM
    translations/slabel_zh_CN.ts
    translations/slabel_en.ts)

# ---- 主题与翻译：构建时拷贝到输出目录 ----
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/themes/dark.qss
           ${CMAKE_BINARY_DIR}/themes/default.qss
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/themes
        ${CMAKE_BINARY_DIR}/themes
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/themes/dark.qss
            ${CMAKE_CURRENT_SOURCE_DIR}/themes/default.qss
    COMMENT "Copying theme files to build directory"
)

add_custom_target(slabel_themes DEPENDS
    ${CMAKE_BINARY_DIR}/themes/dark.qss
    ${CMAKE_BINARY_DIR}/themes/default.qss)

add_custom_target(slabel_translations ALL
    DEPENDS ${SLABEL_QM}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/translations
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SLABEL_QM} ${CMAKE_BINARY_DIR}/translations
    COMMENT "Copying translation .qm files to build directory"
)

# ---- 库目标 ----
add_library(slabel SHARED ${SLABEL_SOURCES} ${SLABEL_HEADERS})
target_compile_definitions(slabel PRIVATE SLABEL_BUILD)
target_include_directories(slabel PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(slabel PUBLIC Qt5::Widgets)

# ---- 构建库前确保主题已就位（测试/示例运行时依赖）
add_dependencies(slabel slabel_themes)

# ---- 运行时部署（仅 Windows 需要）----
# Qt DLL 不能脱离安装目录加载（会导致 0xc0000602）。
# 解决方案：拷贝 Qt DLL + 生成 qt.conf 指定 Prefix 指向 Qt 安装目录，
# 这样 Qt 知道去哪找插件、翻译等资源。MinGW 运行时也一并拷贝。
# 非 Windows 平台没有对应的 .dll/qwindows.dll，整块跳过
# （同 standard_lable.pro 里 qmake 侧的 win32 {} 限定，避免在 Linux 上构建失败）。
if(WIN32)
get_target_property(_qt_core_lib Qt5::Core LOCATION)
get_filename_component(_qt_bin_dir "${_qt_core_lib}" DIRECTORY)
get_filename_component(_qt_prefix  "${_qt_bin_dir}/.." ABSOLUTE)
set(_qt_plugins_src "${_qt_prefix}/plugins")

set(_qt_dlls
    Qt5Core Qt5Gui Qt5Widgets Qt5Test
)
set(_mingw_dlls
    libgcc_s_seh-1 libstdc++-6 libwinpthread-1
)

add_custom_target(deploy_runtime ALL
    COMMENT "Deploying Qt/MinGW runtime DLLs + qt.conf")

# Qt DLL
foreach(_dll ${_qt_dlls})
    add_custom_command(TARGET deploy_runtime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_qt_bin_dir}/${_dll}.dll"
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/"
        COMMENT "Deploy ${_dll}.dll")
endforeach()

# MinGW 运行时 DLL
foreach(_dll ${_mingw_dlls})
    add_custom_command(TARGET deploy_runtime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_qt_bin_dir}/${_dll}.dll"
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/"
        COMMENT "Deploy ${_dll}.dll")
endforeach()

# Qt 平台插件
add_custom_command(TARGET deploy_runtime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/platforms"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${_qt_plugins_src}/platforms/qwindows.dll"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/platforms/"
    COMMENT "Deploy Qt platform plugin")

# qt.conf：告诉 Qt 安装目录在哪，否则 Qt DLL 在构建目录下找不到资源
add_custom_command(TARGET deploy_runtime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "[Paths]" > "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qt.conf"
    COMMAND ${CMAKE_COMMAND} -E echo "Prefix = ${_qt_prefix}" >> "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qt.conf"
    COMMENT "Generating qt.conf")
endif()

enable_testing()
add_subdirectory(tests)
add_subdirectory(examples)
