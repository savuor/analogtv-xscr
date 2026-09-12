cmake_minimum_required(VERSION 3.5)

project(analogtv-xscr C CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wpedantic -pedantic-errors -Wall -Wno-overlength-strings \
                                        -Werror -Wextra -Wcast-align -Wcast-qual -Wfloat-equal -Wredundant-decls \
                                        -Wformat=2 -Wlogical-op -Wmissing-include-dirs -Wpointer-arith \
                                        -Wsequence-point -Wshadow -Wswitch -Wundef -Wunreachable-code \
                                        -Wunused-but-set-parameter -Wwrite-strings -std=c++17")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall -Wno-overlength-strings -Wshadow -std=c++17")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CMAKE_CXX_FLAGS} -O2")

find_package(OpenCV)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)
find_package(nlohmann_json REQUIRED)

set(CMAKE_AUTOMOC ON)

#qt_standard_project_setup()

include_directories(
    "./src"
    ${OpenCV_INCLUDE_DIRS}
    ${Qt6Core_INCLUDE_DIRS}
    ${Qt6Widgets_INCLUDE_DIRS}
)

add_executable(analogtv-cli
    "./src/analogtv-cli.cpp"
    "./src/source.cpp"
    "./src/output.cpp"
    "./src/analogtv.cpp"
    "./src/analogtv_input.cpp"
    "./src/utils.cpp"
    "./src/control.cpp"
    "./src/random_control.cpp"
)

target_link_libraries(analogtv-cli PRIVATE ${OpenCV_LIBS})

add_executable(analogtv-gui
    "./src/analogtv-gui.cpp"
    "./src/json_settings.cpp"
    "./src/control_window.cpp"
    "./src/source.cpp"
    "./src/output.cpp"
    "./src/analogtv.cpp"
    "./src/analogtv_input.cpp"
    "./src/utils.cpp"
)

target_link_libraries(analogtv-gui PRIVATE ${OpenCV_LIBS} Qt6::Core Qt6::Widgets nlohmann_json::nlohmann_json)
