cmake_minimum_required(VERSION 3.19)
project(M2MConvertor VERSION 0.1 LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets LinguistTools)
find_package(OpenSSL 1.1.1 REQUIRED COMPONENTS Crypto)

qt_standard_project_setup()

qt_add_executable(M2MConvertor
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    domain/automaton.cpp
    domain/automaton.h
    domain/conversionservice.cpp
    domain/conversionservice.h
    domain/equivalencechecker.cpp
    domain/equivalencechecker.h
    domain/isomorphismchecker.cpp
    domain/isomorphismchecker.h
    infrastructure/variantarchive.cpp
    infrastructure/variantarchive.h
    infrastructure/progressstore.cpp
    infrastructure/progressstore.h
    ui/automatoncanvas.cpp
    ui/automatoncanvas.h
    ui/automatoneditor.cpp
    ui/automatoneditor.h
)

target_include_directories(M2MConvertor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_translations(
    TARGETS M2MConvertor
    TS_FILES M2MConvertor_ru_RU.ts
)

target_link_libraries(M2MConvertor
    PRIVATE
        Qt::Core
        Qt::Widgets
        OpenSSL::Crypto
)

include(CTest)
if(BUILD_TESTING)
    qt_add_executable(M2MConvertorTests
        tests/domain_tests.cpp
        domain/automaton.cpp
        domain/conversionservice.cpp
        domain/equivalencechecker.cpp
        domain/isomorphismchecker.cpp
        infrastructure/progressstore.cpp
        infrastructure/variantarchive.cpp
    )
    target_include_directories(M2MConvertorTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(M2MConvertorTests PRIVATE Qt::Core OpenSSL::Crypto)
    add_test(NAME M2MConvertorDomain COMMAND M2MConvertorTests)

    qt_add_executable(M2MConvertorGuiSmoke
        tests/gui_smoke.cpp
        mainwindow.cpp
        mainwindow.h
        domain/automaton.cpp
        domain/conversionservice.cpp
        domain/equivalencechecker.cpp
        domain/isomorphismchecker.cpp
        infrastructure/variantarchive.cpp
        infrastructure/progressstore.cpp
        ui/automatoncanvas.cpp
        ui/automatoncanvas.h
        ui/automatoneditor.cpp
        ui/automatoneditor.h
    )
    target_include_directories(M2MConvertorGuiSmoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(M2MConvertorGuiSmoke PRIVATE Qt::Core Qt::Widgets OpenSSL::Crypto)
    add_test(NAME M2MConvertorGuiStartup COMMAND M2MConvertorGuiSmoke)
    set_tests_properties(M2MConvertorGuiStartup PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

install(TARGETS M2MConvertor
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET M2MConvertor
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
