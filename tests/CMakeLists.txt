# 测试需要 AUTOMOC 以处理 #include "test_xxx.moc"（Q_OBJECT 定义在 .cpp 中）
set(CMAKE_AUTOMOC ON)

# 测试运行时 DLL 搜索路径：项目产物 + Qt bin + MinGW bin
get_target_property(_qtcore_loc Qt5::Core LOCATION)
get_filename_component(QT_BIN_DIR "${_qtcore_loc}" DIRECTORY)
get_filename_component(MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

function(slabel_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE slabel Qt5::Test Qt5::Widgets)
    add_test(NAME ${name} COMMAND ${name})
    set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen")
    set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT
        "PATH=${CMAKE_BINARY_DIR}\\;${QT_BIN_DIR}\\;${MINGW_BIN_DIR}\\;$ENV{PATH}")
endfunction()

slabel_test(test_controlcore)
slabel_test(test_thememanager)
slabel_test(test_scontrol_theme)
slabel_test(test_language)
target_compile_definitions(test_language PRIVATE TRANS_DIR="${CMAKE_BINARY_DIR}/translations")
slabel_test(test_binding)
slabel_test(test_controls)
slabel_test(test_theme_files)
target_compile_definitions(test_theme_files PRIVATE THEME_DIR="${CMAKE_BINARY_DIR}/themes")
slabel_test(test_opstate)
slabel_test(test_control_bridge)
slabel_test(test_font_tokens)
target_compile_definitions(test_font_tokens PRIVATE THEME_DIR="${CMAKE_BINARY_DIR}/themes")
slabel_test(test_control_engine)
slabel_test(test_version)
