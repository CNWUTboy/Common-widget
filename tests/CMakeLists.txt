function(slabel_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE slabel Qt5::Test Qt5::Widgets)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

slabel_test(test_controlcore)
slabel_test(test_thememanager)
