cmake_minimum_required(VERSION 3.19)
project(Automata LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Gui)

qt_standard_project_setup()

qt_add_executable(Automata
    WIN32
    main.cpp
)

target_link_libraries(Automata
    PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Widgets
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
