# SSwatch.h 和 widgets/SXxx.h 均含 Q_OBJECT，需纳入源列表供 AUTOMOC 发现
set(CMAKE_AUTORCC ON)
add_executable(slabel_gallery
    main.cpp
    Gallery.cpp
    Gallery.h
    SSwatch.h
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
