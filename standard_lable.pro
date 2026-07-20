TEMPLATE = lib
TARGET = slabel
QT += widgets
CONFIG += c++17 shared
DEFINES += SLABEL_BUILD
INCLUDEPATH += include
HEADERS += $$files(include/slabel/*.h)
SOURCES += $$files(src/*.cpp)
