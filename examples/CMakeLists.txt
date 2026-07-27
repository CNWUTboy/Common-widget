# SSwatch.h 和 widgets/SXxx.h 均含 Q_OBJECT，需纳入源列表供 AUTOMOC 发现
set(CMAKE_AUTORCC ON)
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
    resources/icons.qrc
)
target_link_libraries(slabel_gallery PRIVATE slabel Qt5::Widgets)
target_compile_definitions(slabel_gallery PRIVATE
    THEME_DIR="${CMAKE_BINARY_DIR}/themes"
    TRANS_DIR="${CMAKE_BINARY_DIR}/translations")

# ---- 零接入全局管理演示（独立可执行）----
# 演示专用翻译：en.ts -> en.qm（生成到 examples 构建目录）
qt5_add_translation(DEMO_QM translations/en.ts)
add_custom_target(slabel_global_demo_qm ALL DEPENDS ${DEMO_QM})

add_executable(slabel_global_demo global_demo.cpp)
add_dependencies(slabel_global_demo slabel_global_demo_qm slabel_themes)
target_link_libraries(slabel_global_demo PRIVATE slabel Qt5::Widgets)
target_compile_definitions(slabel_global_demo PRIVATE
    THEME_DIR="${CMAKE_BINARY_DIR}/themes"
    DEMO_LANG_DIR="${CMAKE_CURRENT_BINARY_DIR}"
    DEMO_TS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/translations")
