cmake_minimum_required(VERSION 3.31)
project(pedalplantengine)

set(CMAKE_CXX_STANDARD 26)

set(CMAKE_PREFIX_PATH "C:/Qt/Qt5.14.2/5.14.2/mingw73_64/lib/cmake")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)

include_directories(ASIOSDK ASIOSDK/common ASIOSDK/host ASIOSDK/host/pc src include)
add_executable(pedalplantengine
        src/main.cpp
        ASIOSDK/common/asio.cpp
        ASIOSDK/host/asiodrivers.cpp
        ASIOSDK/host/ASIOConvertSamples.cpp
        ASIOSDK/host/pc/asiolist.cpp
        src/fx/fx.cpp
        src/fx/fx.hpp
        src/fx/fx_xpass.hpp
        src/wav.cpp
        src/wav.hpp
        src/fx/fx_amplitude.hpp
        src/fx/fx_mix.hpp
        src/fx/fx_curve.hpp
        src/fx_gfx.cpp
        src/fx_gfx.hpp
        src/fx/widgets/fx_widgets_amplitude.cpp
        src/fx/widgets/fx_widgets_amplitude.hpp
        src/main.hpp
        src/fx/widgets/fx_widgets_xpass.cpp
        src/fx/widgets/fx_widgets_xpass.hpp
        #        ASIOSDK/host/sample/hostsample.cpp
)

target_link_libraries(pedalplantengine
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
)

if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        endif ()
    endif ()
    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
    endif ()
    foreach (QT_LIB Core Gui Widgets)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt5${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach (QT_LIB)
endif ()