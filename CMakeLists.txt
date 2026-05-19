cmake_minimum_required(VERSION 3.16)
project(AurudinhaVN)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Multimedia TextToSpeech)

add_executable(aurudinha_game src/game.cpp)
target_link_libraries(aurudinha_game Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Multimedia Qt6::TextToSpeech)
target_compile_options(aurudinha_game PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

add_custom_command(TARGET aurudinha_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:aurudinha_game>/assets"
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:aurudinha_game>/config"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/config"
            "$<TARGET_FILE_DIR:aurudinha_game>/config"
    COMMENT "Copying runtime assets and config next to aurudinha_game"
)
