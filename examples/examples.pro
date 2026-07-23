# examples.pro — slabel 控件展示程序 (qmake)
QT += widgets
TEMPLATE = app
CONFIG += c++17 console
TARGET = slabel_gallery

INCLUDEPATH += ../include

# 输出到库的 DESTDIR，让 .exe 与 .dll（以及已拷贝的主题/翻译/Qt 运行时）在同一目录。
# 库按 README 记载的方式原地构建（qmake standard_lable.pro && make，在项目根目录下
# 执行），其 DESTDIR=bin 落在项目根的 bin/ 下，因此这里用 ../bin 与之对齐——
# 和 tests/*.pro 里 `LIBS += -L../bin -lslabel` 保持同一约定，不假设任何影子构建目录层级。
DESTDIR = ../bin
LIBS += -L$$DESTDIR -lslabel

# Linux 下动态链接器不会默认搜索可执行文件自身所在目录，仅靠 -L 只解决链接期找库，
# 运行期仍会报 "cannot open shared object file"；写入 $ORIGIN 相对 rpath，
# 让可执行文件在其自身目录（即 libslabel.so 所在的 ../bin）里查找运行时依赖。
unix: QMAKE_LFLAGS += -Wl,-rpath,\'\$$ORIGIN\'

SOURCES += main.cpp \
           Gallery.cpp

HEADERS += Gallery.h \
           SSwatch.h \
           widgets/SButton.h \
           widgets/SCheckBox.h \
           widgets/SComboBox.h \
           widgets/SDialog.h \
           widgets/SGroupBox.h \
           widgets/SLabel.h \
           widgets/SLineEdit.h \
           widgets/SListView.h \
           widgets/SProgressBar.h \
           widgets/SRadioButton.h \
           widgets/SSlider.h \
           widgets/SSpinBox.h \
           widgets/STabWidget.h \
           widgets/STableView.h \
           widgets/STreeView.h

RESOURCES += resources/icons.qrc

# 告诉示例主题和翻译的加载路径（必须是绝对路径，因为运行时 cwd 不确定）。
# 与上面的 DESTDIR 指向同一个目录（项目根的 bin/），$$_PRO_FILE_PWD_ 恒等于
# examples.pro 所在源码目录，不受实际构建目录影响。
THEMES_PATH = $$_PRO_FILE_PWD_/../bin/themes
TRANS_PATH  = $$_PRO_FILE_PWD_/../bin/translations
DEFINES += THEME_DIR=\\\"$$shell_path($$THEMES_PATH)\\\"
DEFINES += TRANS_DIR=\\\"$$shell_path($$TRANS_PATH)\\\"
