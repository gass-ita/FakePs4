# ==============================================================================
# FakePs4 - Pure C++ Core & Qt GUI Project (CMake Migration)
# ==============================================================================

# 1. Minimum CMake version required
cmake_minimum_required(VERSION 3.16)

# 2. Project Configuration (Replaces TARGET = FakePs4)
project(FakePs4 VERSION 1.0 LANGUAGES CXX)

# Enforce C++17 (Required for modern Qt6 compilation)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Handle Qt Meta-Object Compiler automatically (Replaces OBJECTS_DIR and MOC_DIR)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Redirect compiled executable (Replaces DESTDIR = ./bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# 3. Find Qt Libraries (Replaces QT += core gui widgets)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent)

# 4. Handle Wildcards (Replaces src/core/Shapes/*.cc and src/core/Tools/*.cc)
# CONFIGURE_DEPENDS ensures CMake actively updates if you add/remove files in these directories
file(GLOB SHAPE_SOURCES CONFIGURE_DEPENDS "src/core/Shapes/*.cc")
file(GLOB TOOL_SOURCES CONFIGURE_DEPENDS "src/core/Tools/*.cc")
file(GLOB LAYER_SOURCES CONFIGURE_DEPENDS "src/core/Layer/*.cc")
file(GLOB LAYER_HEADERS CONFIGURE_DEPENDS "src/core/include/Layer/*.h")
file(GLOB FILTER_SOURCES CONFIGURE_DEPENDS "src/core/Filter/*.cc")
file(GLOB FILTER_HEADERS CONFIGURE_DEPENDS "src/core/include/Filter/*.h")

# 5. Define the Executable and Source Files (Replaces SOURCES += and HEADERS +=)
# CMake handles .cc extensions automatically without needing a QMAKE_EXT_CPP flag
add_executable(FakePs4
    src/main.cc
    ${LAYER_SOURCES}
    src/core/LayerManager.cc
    ${SHAPE_SOURCES}
    ${TOOL_SOURCES}
    src/core/FileSaving.cc
    src/gui/Canvas.cc
    src/gui/MainWindow.cc
    ${FILTER_SOURCES}
    
    # Header files are added here so they appear cleanly inside your IDE file trees
    ${LAYER_HEADERS}
    ${FILTER_HEADERS}
    src/core/include/LayerManager.h
    src/core/include/LMObserver.h
    src/core/include/Shape.h
    src/gui/include/Canvas.h
    src/gui/include/MainWindow.h
    src/core/include/Filter/Filter.h
    src/core/include/Filter/ConvolutionFilter.h
)

# 6. Include Paths (Replaces INCLUDEPATH +=)
target_include_directories(FakePs4 PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core/include/Filter
    ${CMAKE_SOURCE_DIR}/src/core/include
    ${CMAKE_SOURCE_DIR}/src/core/include/Layer
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/gui/include
    ${CMAKE_SOURCE_DIR}/src/gui
)

# 7. Link Qt Libraries to the target
target_link_libraries(FakePs4 PRIVATE 
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets
    Qt6::Concurrent
)