cmake_minimum_required(VERSION 3.16)

project(fake_kdeconnect LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core DBus Gui)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core DBus Gui)

add_executable(fake-kdeconnect
  main.cpp
  kdeconnect_interfaces/conversationmessage_ext.h kdeconnect_interfaces/conversationmessage_ext.cpp kdeconnect_interfaces/kdeconnect_proxy.h kdeconnect_interfaces/kdeconnect_proxy.cpp kdeconnect_interfaces/conversationmessage.cpp kdeconnect_interfaces/conversationmessage.h
  dbus.h
  deviceconfig.h deviceconfig.cpp
  fakedeviceconversationsinterface.h fakedeviceconversationsinterface.cpp
  fakekdeconnectdevice.h fakekdeconnectdevice.cpp
  fakekdeconnectdaemon.h fakekdeconnectdaemon.cpp
  commands.h commands.cpp
  harvester.h harvester.cpp
  nameresolver.h nameresolver.cpp
  fakekdeconnectbattery.h fakekdeconnectbattery.cpp
)
target_link_libraries(fake-kdeconnect PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::DBus Qt${QT_VERSION_MAJOR}::Gui)


include(GNUInstallDirs)
install(TARGETS fake-kdeconnect
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
