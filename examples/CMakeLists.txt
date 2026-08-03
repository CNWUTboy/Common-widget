# SSwatch.h 和 widgets/SXxx.h 均含 Q_OBJECT，需纳入源列表供 AUTOMOC 发现
set(CMAKE_AUTORCC ON)

# ---- 资源嵌入：把主题/翻译打进可执行文件，避免安装/移动后按绝对路径找不到 ----
# 主题 .qss（源文件，直接用源码 qrc）
set(THEMES_QRC "${CMAKE_SOURCE_DIR}/themes/themes.qrc")

# 生成一个引用 .qm 的 qrc（.qm 为构建产物，用绝对路径写入 qrc；configure 时立即写好，
# rcc 在构建时读取内容，依赖链由 CMake 依据 .qm 输出自动串接）。
function(slabel_make_qm_qrc out_qrc prefix)
    set(_content "<!DOCTYPE RCC>\n<RCC version=\"1.0\">\n  <qresource prefix=\"/${prefix}\">\n")
    foreach(_qm ${ARGN})
        get_filename_component(_name "${_qm}" NAME)
        string(APPEND _content "    <file alias=\"${_name}\">${_qm}</file>\n")
    endforeach()
    string(APPEND _content "  </qresource>\n</RCC>\n")
    file(WRITE "${out_qrc}" "${_content}")
endfunction()

# ---- Gallery：嵌入三套主题 + 顶层生成的 slabel_*.qm（翻译） ----
slabel_make_qm_qrc("${CMAKE_CURRENT_BINARY_DIR}/gallery_qm.qrc" translations ${SLABEL_QM})
qt5_add_resources(GALLERY_RES
    ${THEMES_QRC}
    "${CMAKE_CURRENT_BINARY_DIR}/gallery_qm.qrc")

add_executable(slabel_gallery
    main.cpp
    Gallery.cpp
    Gallery.h
    SSwatch.h
    StatusLight.h
    AccentButton.h
    widgets/SButton.h
    widgets/SCheckBox.h
    widgets/SComboBox.h
    widgets/SDialog.h
    widgets/SGroupBox.h
    widgets/SLabel.h
    widgets/SLineEdit.h
    widgets/SListView.h
    widgets/SProgressBar.h
    widgets/SRadioButton.h
    widgets/SSlider.h
    widgets/SSpinBox.h
    widgets/STabWidget.h
    widgets/STableView.h
    widgets/STreeView.h
    ${GALLERY_RES}
)
target_link_libraries(slabel_gallery PRIVATE slabel Qt5::Widgets)

# ---- 零接入全局管理演示（独立可执行）----
# 演示专用翻译：en.ts -> en.qm（生成到 examples 构建目录）
qt5_add_translation(DEMO_QM translations/en.ts)

# 嵌入：三套主题 + 演示 en.qm + 演示 en.ts（反查目录用）
slabel_make_qm_qrc("${CMAKE_CURRENT_BINARY_DIR}/demo_qm.qrc" translations ${DEMO_QM})
qt5_add_resources(DEMO_RES
    ${THEMES_QRC}
    "${CMAKE_CURRENT_SOURCE_DIR}/translations/ts.qrc"
    "${CMAKE_CURRENT_BINARY_DIR}/demo_qm.qrc")

add_executable(slabel_global_demo global_demo.cpp ${DEMO_RES})
target_link_libraries(slabel_global_demo PRIVATE slabel Qt5::Widgets)
