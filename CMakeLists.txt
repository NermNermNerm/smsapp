cmake_minimum_required(VERSION 3.16)

project(smsapp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_GENERATE_QMLLS_INI ON)

find_package(Qt6 REQUIRED COMPONENTS Quick DBus)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(appsmsapp
    main.cpp
    backend/SmsBackend.cpp
    kdeconnect_proxy.cpp
)

qt_add_qml_module(appsmsapp
    URI smsapp
    VERSION 1.0
    QML_FILES
        Main.qml
)

# Just to make it visible in the ide
add_custom_target(make_proxy_script
    SOURCES make-proxy.py
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appsmsapp PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appsmsapp
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(appsmsapp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(appsmsapp
    PRIVATE Qt6::Quick Qt6::DBus
)

install(TARGETS appsmsapp
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
