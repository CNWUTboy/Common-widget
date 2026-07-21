# slabel 动态库 —— qmake 构建（与 CMake 产物一致）
TEMPLATE = lib
TARGET = slabel
QT += widgets
CONFIG += c++17 shared
DEFINES += SLABEL_BUILD
INCLUDEPATH += include
HEADERS += $$files(include/slabel/*.h)
SOURCES += $$files(src/*.cpp)

# 主题 QSS 与语言 .ts/.qm 为运行期资源（themes/、translations/），
# 由应用按路径加载，不编入库；.qm 生成见 CMake 或手动 lrelease。
