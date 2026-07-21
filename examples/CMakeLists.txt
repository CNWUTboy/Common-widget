add_executable(slabel_gallery main.cpp Gallery.cpp SSwatch.h)
target_link_libraries(slabel_gallery PRIVATE slabel Qt5::Widgets)
target_compile_definitions(slabel_gallery PRIVATE
    THEME_DIR="${CMAKE_BINARY_DIR}/themes"
    TRANS_DIR="${CMAKE_BINARY_DIR}/translations")
