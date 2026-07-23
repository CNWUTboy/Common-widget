# tests.pro — qmake 测试伞项目
# 使用方式：先构建库（../standard_lable.pro），再打开此文件构建测试。
TEMPLATE = subdirs

SUBDIRS = \
    test_controlcore \
    test_thememanager \
    test_scontrol_theme \
    test_language \
    test_binding \
    test_controls \
    test_theme_files \
    test_opstate \
    test_control_bridge \
    test_font_tokens
