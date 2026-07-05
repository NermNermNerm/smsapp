cmake_minimum_required(VERSION 3.16)

project(smsapp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_GENERATE_QMLLS_INI ON)

find_package(Qt6 REQUIRED COMPONENTS Quick DBus Gui Widgets)
find_library(PHONENUMBER_LIB phonenumber)
message(STATUS "PHONENUMBER_LIB = ${PHONENUMBER_LIB}")

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(appsmsapp
    main.cpp

    kdeconnect_proxy.cpp
)

qt_add_qml_module(appsmsapp
    URI smsapp
    VERSION 1.0
    SOURCES TrayIconController.h
    SOURCES TrayIconController.cpp
    SOURCES kdeconnect_interfaces/conversationmessage.h kdeconnect_interfaces/conversationmessage.cpp
    SOURCES backend/conversationheader.h backend/conversationheader.cpp
    SOURCES backend/conversationlistmodel.h backend/conversationlistmodel.cpp
    SOURCES backend/messagelistmodel.h backend/messagelistmodel.cpp
    SOURCES backend/messageitem.h backend/messageitem.cpp
    SOURCES backend/nameresolver.h backend/nameresolver.cpp
    SOURCES backend/cachemanager.h backend/cachemanager.cpp
    SOURCES kdeconnect_interfaces/conversationmessage_ext.h kdeconnect_interfaces/conversationmessage_ext.cpp
    SOURCES backend/messageshandler.h backend/messageshandler.cpp
    SOURCES backend/devicestatus.h backend/devicestatus.cpp
    SOURCES dbus.h
    SOURCES settings.h settings.cpp
    RESOURCES todo.md
    QML_FILES Main.qml
    QML_FILES StatusBar.qml
    QML_FILES ConversationList.qml
    QML_FILES MessageList.qml
)

# Add general Qt resources *after* the QML module
qt_add_resources(appsmsapp "app_resources"
    PREFIX "/"
    FILES
        icons/tray_connected_msgs.svg
        icons/tray_disconnected_msgs.svg
        icons/tray_error_msgs.svg
        icons/tray_connected_nomsgs.svg
        icons/tray_disconnected_nomsgs.svg
        icons/tray_error_nomsgs.svg
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
    PRIVATE
        Qt6::Quick
        Qt6::DBus
        Qt6::Widgets
        Qt6::Gui
        ${PHONENUMBER_LIB}
)

install(TARGETS appsmsapp
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
