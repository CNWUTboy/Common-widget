QT += testlib widgets
TEMPLATE = app
CONFIG += c++17 testcase console
TARGET = test_scontrol_theme

INCLUDEPATH += ../include
LIBS += -L../bin -lslabel

SOURCES = test_scontrol_theme.cpp
