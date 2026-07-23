# slabel 动态库 —— qmake 构建（与 CMake 产物一致）
TEMPLATE = lib
TARGET = slabel
QT += widgets
CONFIG += c++17 shared

DEFINES += SLABEL_BUILD
INCLUDEPATH += include

HEADERS += $$files(include/slabel/*.h)
SOURCES += $$files(src/*.cpp)

# ---- 翻译：声明 .ts 文件；qmake 阶段调用 lrelease 生成 .qm ----
TRANSLATIONS += translations/slabel_zh_CN.ts \
                translations/slabel_en.ts

# 在 qmake 阶段生成 .qm（构建时再拷到输出目录）
system(cd \"$$_PRO_FILE_PWD_\" && lrelease $$TRANSLATIONS): \
    message(Translations compiled.)
else: \
    message(Warning: lrelease failed or not found — .qm files may be missing.)

# ---- 输出目录（保持源码目录整洁）----
MOC_DIR   = build/moc
OBJECTS_DIR = build/obj
RCC_DIR   = build/rcc
DESTDIR   = bin

# ---- 构建后拷贝：主题 & 翻译 .qm 到输出目录 ----
# MinGW make 通过 MSYS sh 执行命令；MSVC nmake 通过 cmd.exe。
# 主题/翻译源目录：用 $$_PRO_FILE_PWD_（恒等于本 .pro 文件所在源码目录，
# 不随 qmake 实际的构建/影子构建目录变化）推导，避免依赖任何构建目录层级假设。
THEMES_SRC = $$shell_path($$_PRO_FILE_PWD_/themes/*.qss)
TRANS_SRC  = $$shell_path($$_PRO_FILE_PWD_/translations/*.qm)

win32-g++ {
    MKDIR_CMD = mkdir -p
    COPY_CMD = cp
    # sh: 文件不存在时跳过
    COPY_QM = -cp $$TRANS_SRC
} else:win32-msvc* {
    MKDIR_CMD = mkdir
    MKDIR_ERR = 2>nul || rem
    COPY_CMD = copy /y
    # cmd: 文件不存在时跳过
    COPY_QM = if exist $$TRANS_SRC copy /y $$TRANS_SRC
} else {
    MKDIR_CMD = mkdir -p
    COPY_CMD = cp
    COPY_QM = -cp $$TRANS_SRC
}

# ---- 运行时部署 ----
# Qt DLL 不能脱离安装目录加载（0xc0000602）。拷贝 DLL + 生成 qt.conf 指定 Prefix。
QT_BIN = $$[QT_INSTALL_BINS]
QT_PLUGINS = $$[QT_INSTALL_PLUGINS]
QT_PREFIX = $$[QT_INSTALL_PREFIX]

QMAKE_POST_LINK += $$MKDIR_CMD $$MKDIR_ERR \"$$shell_path($${DESTDIR}/themes)\"   $$escape_expand(\\n\\t)
QMAKE_POST_LINK += $$MKDIR_CMD $$MKDIR_ERR \"$$shell_path($${DESTDIR}/translations)\" $$escape_expand(\\n\\t)

# 主题 & 翻译（Windows/Linux 都需要）
# 注意：源路径含通配符，不能加引号——sh/cmd 都需要在未加引号时才会展开 *.qss/*.qm。
QMAKE_POST_LINK += $$COPY_CMD $$THEMES_SRC \"$$shell_path($${DESTDIR}/themes)\" $$escape_expand(\\n\\t)
QMAKE_POST_LINK += $$COPY_QM \"$$shell_path($${DESTDIR}/translations)\" $$escape_expand(\\n\\t)

# 以下 Qt/MinGW 运行时 DLL、平台插件、qt.conf 部署只有 Windows 需要；
# 之前未加 win32 限定，导致这些命令在 Linux 上因找不到 .dll 文件而以非零状态退出，
# 中止了整条 QMAKE_POST_LINK 命令链（含它之前的主题/翻译拷贝都被连带判定为构建失败）。
win32 {
    QMAKE_POST_LINK += $$MKDIR_CMD $$MKDIR_ERR \"$$shell_path($${DESTDIR}/platforms)\" $$escape_expand(\\n\\t)

    # Qt DLL
    QMAKE_POST_LINK += $$COPY_CMD \"$$shell_path($${QT_BIN}/Qt5Core.dll)\"    \"$$shell_path($${DESTDIR})\" $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$COPY_CMD \"$$shell_path($${QT_BIN}/Qt5Gui.dll)\"     \"$$shell_path($${DESTDIR})\" $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$COPY_CMD \"$$shell_path($${QT_BIN}/Qt5Widgets.dll)\" \"$$shell_path($${DESTDIR})\" $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$COPY_CMD \"$$shell_path($${QT_BIN}/Qt5Test.dll)\"    \"$$shell_path($${DESTDIR})\" $$escape_expand(\\n\\t)

    # MinGW 运行时 DLL
    QMAKE_POST_LINK += $$COPY_CMD \"$$shell_path($${QT_BIN}/libgcc_s_seh-1.dll)\"  \"$$shell_path($${DESTDIR})\" $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$COPY_CMD \"$$shell_path($${QT_BIN}/libstdc++-6.dll)\"     \"$$shell_path($${DESTDIR})\" $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$COPY_CMD \"$$shell_path($${QT_BIN}/libwinpthread-1.dll)\" \"$$shell_path($${DESTDIR})\" $$escape_expand(\\n\\t)

    # Qt 平台插件
    QMAKE_POST_LINK += $$COPY_CMD \"$$shell_path($${QT_PLUGINS}/platforms/qwindows.dll)\" \"$$shell_path($${DESTDIR}/platforms)\" $$escape_expand(\\n\\t)

    # qt.conf：告知 Qt 安装目录，确保 Qt DLL 在构建目录下也能找到资源
    QMAKE_POST_LINK += echo \"[Paths]\"              >  \"$$shell_path($${DESTDIR}/qt.conf)\"  $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += echo \"Prefix = $$QT_PREFIX\" >> \"$$shell_path($${DESTDIR}/qt.conf)\"
}
