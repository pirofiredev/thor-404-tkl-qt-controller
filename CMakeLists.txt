cmake_minimum_required(VERSION 3.16)
project(thor-404-tkl-qt-controller LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

find_package(PkgConfig REQUIRED)
# Allow finding user-installed or system hidapi-hidraw
list(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/.local" "$ENV{HOME}/.local/usr")
set(ENV{PKG_CONFIG_PATH} "$ENV{HOME}/.local/usr/lib/x86_64-linux-gnu/pkgconfig:$ENV{PKG_CONFIG_PATH}")

pkg_check_modules(HIDAPI REQUIRED hidapi-hidraw)

add_executable(thor-404-controller
    src/main.cpp
    src/devicemanager.h
    src/devicemanager.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/mqttservice.h
    src/mqttservice.cpp
    src/sysfsledservice.h
    src/sysfsledservice.cpp
    src/interactivekeyboardwidget.h
    src/interactivekeyboardwidget.cpp
    resources.qrc
)

target_include_directories(thor-404-controller PRIVATE
    ${HIDAPI_INCLUDE_DIRS}
    src
)

target_link_libraries(thor-404-controller PRIVATE
    Qt6::Widgets
    Qt6::Network
    ${HIDAPI_LIBRARIES}
)

target_link_directories(thor-404-controller PRIVATE
    ${HIDAPI_LIBRARY_DIRS}
)

# Test target for packet framing check
add_executable(test-framing
    tests/test_framing.cpp
    src/devicemanager.h
    src/devicemanager.cpp
)

target_include_directories(test-framing PRIVATE
    ${HIDAPI_INCLUDE_DIRS}
    src
)

target_link_libraries(test-framing PRIVATE
    ${HIDAPI_LIBRARIES}
)

target_link_directories(test-framing PRIVATE
    ${HIDAPI_LIBRARY_DIRS}
)

# Install rules
install(TARGETS thor-404-controller DESTINATION bin)
install(FILES 99-genesis-thor.rules DESTINATION /lib/udev/rules.d/)
install(FILES thor-404-controller.desktop DESTINATION share/applications)
install(FILES resources/genesis-logo.svg DESTINATION share/icons/hicolor/scalable/apps/ RENAME thor-404-controller.svg)

# CPack Packaging Configuration
set(CPACK_PACKAGE_NAME "thor-404-controller")
set(CPACK_PACKAGE_VENDOR "Genesis Linux Community")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Native Linux Qt6/C++20 Controller for Genesis Thor 404 TKL Keyboard")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_CONTACT "pirofiredev <support@factory.ai>")
set(CPACK_GENERATOR "DEB;RPM")

# Debian (.deb) Settings
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "pirofiredev")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.34), libstdc++6 (>= 11), libqt6widgets6 (>= 6.2.0), libqt6network6 (>= 6.2.0), libhidapi-hidraw0")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)

# RedHat / Fedora / openSUSE (.rpm) Settings
set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-or-later")
set(CPACK_RPM_PACKAGE_GROUP "Applications/System")
set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase-gui, hidapi")
set(CPACK_RPM_PACKAGE_RELOCATABLE OFF)

include(CPack)
