QT += testlib widgets
TEMPLATE = app
CONFIG += c++17 testcase console
TARGET = test_language

INCLUDEPATH += ../include
LIBS += -L../bin -lslabel

SOURCES = test_language.cpp

# 告诉测试代码翻译文件所在目录（与 CMake 的 TRANS_DIR 等效）
DEFINES += TRANS_DIR=\\\"$$shell_path($${_PRO_FILE_PWD_}/../build/translations)\\\"
