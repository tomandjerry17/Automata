cmake_minimum_required(VERSION 3.19)
project(Automata LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Gui)

qt_add_executable(Automata
    WIN32
    src/main.cpp
    src/core/nfa.h
    src/core/nfa.cpp
    src/core/dfa.h
    src/core/dfa.cpp
    src/core/thompson.h
    src/core/thompson.cpp
    src/core/subset.h
    src/core/subset.cpp
    src/core/tokens.h
    src/core/tokens.cpp
    src/lexer/tokenizer.h
    src/lexer/tokenizer.cpp
    src/parser/parser.h
    src/parser/parser.cpp
    src/parser/grammar.h
    src/parser/grammar.cpp
    src/gui/mainwindow.h
    src/gui/mainwindow.cpp
    src/gui/automataview.h
    src/gui/automataview.cpp
    src/gui/nfaview.h
    src/gui/nfaview.cpp
    src/validator/validator.cpp
    src/validator/validator.h
)

target_include_directories(Automata PRIVATE src)

target_link_libraries(Automata
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

include(GNUInstallDirs)
install(TARGETS Automata
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET Automata
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})