cmake_minimum_required(VERSION 3.16)
project(VisualNovelCPP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets TextToSpeech)

add_executable(vn_app game.cpp)
target_link_libraries(vn_app Qt6::Core Qt6::Gui Qt6::Widgets Qt6::TextToSpeech)
