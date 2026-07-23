# examples.pro — slabel 控件展示程序 (qmake)
QT += widgets
TEMPLATE = app
CONFIG += c++17 console
TARGET = slabel_gallery

INCLUDEPATH += ../include

# 输出到库的 DESTDIR，让 .exe 与 .dll 在同一目录
# examples.pro 在 examples/ 下，构建目录在 build/qmake_examples/
# 因此 ../qmake_build/bin 回到项目根下的 build/qmake_build/bin
DESTDIR = ../qmake_build/bin
LIBS += -L$$DESTDIR -lslabel

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

# 告诉示例主题和翻译的加载路径（必须是绝对路径，因为运行时 cwd 不确定）
THEMES_PATH = $$_PRO_FILE_PWD_/../build/qmake_build/bin/themes
TRANS_PATH  = $$_PRO_FILE_PWD_/../build/qmake_build/bin/translations
DEFINES += THEME_DIR=\\\"$$shell_path($$THEMES_PATH)\\\"
DEFINES += TRANS_DIR=\\\"$$shell_path($$TRANS_PATH)\\\"
