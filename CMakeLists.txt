cmake_minimum_required(VERSION 3.31)
project(pedalplantengine)

set(CMAKE_CXX_STANDARD 26)

include_directories(ASIOSDK ASIOSDK/common ASIOSDK/host ASIOSDK/host/pc src)
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
        #        ASIOSDK/host/sample/hostsample.cpp
)