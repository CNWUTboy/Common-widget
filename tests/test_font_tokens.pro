QT += testlib widgets
TEMPLATE = app
CONFIG += c++17 testcase console
TARGET = test_font_tokens

INCLUDEPATH += ../include
LIBS += -L../bin -lslabel

SOURCES = test_font_tokens.cpp

# 告诉测试代码主题文件所在目录（与 CMake 的 THEME_DIR 等效）
DEFINES += THEME_DIR=\\\"$$shell_path($${_PRO_FILE_PWD_}/../build/themes)\\\"
