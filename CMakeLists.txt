# ==============================================================================
# FakePs4 - Pure C++ Core & Qt GUI Project (CMake Migration)
# ==============================================================================

# 1. Minimum CMake version required
cmake_minimum_required(VERSION 3.16)

# 2. Project Configuration (TARGET = FakePs4)
project(FakePs4 VERSION 1.0 LANGUAGES CXX)

# Enforce C++17 (Required for modern Qt6)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Handle MOC automatically (Replaces MOC_DIR = ./build)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Redirect compiled executable (Replaces DESTDIR = ./bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# 3. Find Qt Libraries (Replaces QT += core gui widgets)
# Change Qt6 to Qt5 here if you are compiling against Qt5
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# 4. Handle Wildcards (Replaces src/core/Shapes/*.cc)
# Note: CMake generally prefers explicit file lists, but GLOB mimics qmake's wildcard behavior
file(GLOB SHAPE_SOURCES "src/core/Shapes/*.cc")
file(GLOB TOOL_SOURCES "src/core/Tools/*.cc")

# 5. Define the Executable and Source Files (Replaces SOURCES += and HEADERS +=)
# QMAKE_EXT_CPP = .cc is handled automatically by passing .cc files here
add_executable(FakePs4
    src/main.cc
    src/core/Layer.cc
    src/core/LayerManager.cc
    ${SHAPE_SOURCES}
    ${TOOL_SOURCES}
    src/core/FileSaving.cc
    src/gui/Canvas.cc
    src/gui/MainWindow.cc
    
    # Adding headers isn't strictly required for compilation, 
    # but it ensures they appear in your IDE (like Qt Creator or VS Code)
    src/core/include/Layer.h
    src/core/include/LayerManager.h
    src/core/include/LMObserver.h
    src/core/include/Shape.h
    src/gui/include/Canvas.h
    src/gui/include/MainWindow.h
)

# 6. Include Paths (Replaces INCLUDEPATH +=)
target_include_directories(FakePs4 PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core/include
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/gui/include
    ${CMAKE_SOURCE_DIR}/src/gui
)

# 7. Link Qt Libraries to the target
target_link_libraries(FakePs4 PRIVATE 
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets
)