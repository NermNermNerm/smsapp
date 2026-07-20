cmake_minimum_required(VERSION 3.16)

project(kpeople_lookup LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- ECM + KDE Frameworks setup (CERTAIN requirement for KF5) ---
find_package(ECM REQUIRED NO_MODULE)
find_package(KF5Contacts REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)

# --- Qt 5 (CERTAIN: KPeople requires Qt 5) ---
find_package(Qt5 REQUIRED COMPONENTS Core DBus)

# --- KF5 People (CERTAIN: this is the correct module) ---
find_package(KF5People REQUIRED)

# --- Executable ---
add_executable(kpeople-lookup
    main.cpp
)

# --- Linking (CERTAIN: these are the correct libs) ---
target_link_libraries(kpeople-lookup
    Qt5::Core
    Qt5::DBus
    KF5::People
    KF5::Contacts
)

install(TARGETS kpeople-lookup
    RUNTIME DESTINATION ${KDE_INSTALL_BINDIR}
)
