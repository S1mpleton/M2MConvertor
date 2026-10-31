cmake_minimum_required(VERSION 3.19)
project(M2MConvertor VERSION 0.2 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets LinguistTools)
find_package(OpenSSL 1.1.1 REQUIRED COMPONENTS Crypto)
qt_standard_project_setup()

add_library(M2MConvertorCore STATIC
    domain/message.h domain/result.cpp domain/result.h
    infrastructure/logger.cpp infrastructure/logger.h
    domain/automaton.cpp domain/automaton.h
    domain/trainingvariant.h domain/reactionchecker.cpp domain/reactionchecker.h
    domain/conversionservice.cpp domain/conversionservice.h
    domain/equivalencechecker.cpp domain/equivalencechecker.h
    domain/isomorphismchecker.cpp domain/isomorphismchecker.h
    infrastructure/variantarchive.cpp infrastructure/variantarchive.h
    infrastructure/progressstore.cpp infrastructure/progressstore.h)
target_include_directories(M2MConvertorCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(M2MConvertorCore PUBLIC Qt::Core Qt::Gui OpenSSL::Crypto)

add_library(M2MConvertorUi STATIC
    ui/widgets/actionbar.cpp ui/widgets/actionbar.h
    ui/contraststyle.cpp ui/contraststyle.h
    infrastructure/translationmanager.cpp infrastructure/translationmanager.h
    ui/uitext.cpp ui/uitext.h ui/logwindow.cpp ui/logwindow.h ui/logwindow.ui
    ui/mealycelldelegate.cpp ui/mealycelldelegate.h
    ui/plaincelldelegate.cpp ui/plaincelldelegate.h
    ui/widgets/workscrollarea.cpp ui/widgets/workscrollarea.h
    ui/widgets/taskimage.h ui/widgets/hintsplitter.cpp ui/widgets/hintsplitter.h ui/workflowtabs.h
    ui/appicons.cpp ui/appicons.h ui/widgets/feedbacklabel.cpp ui/widgets/feedbacklabel.h
    ui/widgets/wrappinglabel.cpp ui/widgets/wrappinglabel.h
    mainwindow.cpp mainwindow.h
    ui/automatongrapheditor.cpp ui/automatongrapheditor.h
    ui/automatoneditor.cpp ui/automatoneditor.h)
target_link_libraries(M2MConvertorUi PUBLIC M2MConvertorCore Qt::Widgets)

qt_add_executable(M2MConvertor WIN32 MACOSX_BUNDLE main.cpp)
target_link_libraries(M2MConvertor PRIVATE M2MConvertorUi)
if(WIN32)
    enable_language(RC)
    target_sources(M2MConvertor PRIVATE resources/windows-icon.rc)
    target_include_directories(M2MConvertor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/resources)
endif()
set(TRANSLATION_TARGETS M2MConvertor)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)
    qt_add_executable(M2MConvertorTests tests/domain_tests.cpp)
    target_link_libraries(M2MConvertorTests PRIVATE M2MConvertorCore)
    add_test(NAME M2MConvertorDomain COMMAND M2MConvertorTests)

    qt_add_executable(M2MConvertorGuiSmoke tests/gui_smoke.cpp)
    target_link_libraries(M2MConvertorGuiSmoke PRIVATE M2MConvertorUi Qt::Test)
    list(APPEND TRANSLATION_TARGETS M2MConvertorGuiSmoke)
    add_test(NAME M2MConvertorGuiStartup COMMAND M2MConvertorGuiSmoke)
    set_tests_properties(M2MConvertorGuiStartup PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

foreach(icon_target IN LISTS TRANSLATION_TARGETS)
    qt_add_resources(${icon_target} window_icons
        PREFIX "/icons" BASE "resources/icons"
        FILES resources/icons/m2mconvertor.png)
endforeach()

get_target_property(CORE_TRANSLATION_SOURCES M2MConvertorCore SOURCES)
get_target_property(UI_TRANSLATION_SOURCES M2MConvertorUi SOURCES)
qt_add_translations(TARGETS ${TRANSLATION_TARGETS}
    TS_FILES M2MConvertor_ru_RU.ts M2MConvertor_en.ts
    SOURCES ${CORE_TRANSLATION_SOURCES} ${UI_TRANSLATION_SOURCES} main.cpp)

install(TARGETS M2MConvertor
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
qt_generate_deploy_app_script(TARGET M2MConvertor
    OUTPUT_SCRIPT deploy_script NO_UNSUPPORTED_PLATFORM_ERROR)
install(SCRIPT ${deploy_script})
