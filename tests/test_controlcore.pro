QT += testlib widgets
TEMPLATE = app
CONFIG += c++17 testcase console
TARGET = test_controlcore

INCLUDEPATH += ../include

# 假设库已构建到 ../bin（与 standard_lable.pro 的 DESTDIR 一致）
LIBS += -L../bin -lslabel

SOURCES = test_controlcore.cpp
