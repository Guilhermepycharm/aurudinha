cmake_minimum_required(VERSION 3.16)
project(AurudinhaVN)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets TextToSpeech)

add_executable(aurudinha_game src/game.cpp)
target_link_libraries(aurudinha_game Qt6::Core Qt6::Gui Qt6::Widgets Qt6::TextToSpeech)
