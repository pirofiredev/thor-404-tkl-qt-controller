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
install(FILES 99-genesis-thor.rules DESTINATION /lib/udev/rules.d/ OPTIONAL)
