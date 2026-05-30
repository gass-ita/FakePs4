# ==============================================================================
# FakePs4 - Pure C++ Core & Qt GUI Project
# ==============================================================================

# 1. Project Configuration
TEMPLATE = app
QT += core gui widgets

TARGET = FakePs4
DESTDIR = ./bin
OBJECTS_DIR = ./build
MOC_DIR = ./build


QMAKE_EXT_CPP = .cc

# 2. Include Paths 
# (This allows you to write #include "Layer.h" instead of full relative paths)
INCLUDEPATH += \
    src/core/include \
    src/core \
    src/gui/include \
    src/gui

# 3. Source Files (Implementations)
SOURCES += \
    src/main.cc \
    src/core/Layer.cc \
    src/core/LayerManager.cc \
    src/core/Shapes/*.cc \ 
    src/core/Tools/*.cc \         
    src/gui/Canvas.cc \
    src/gui/MainWindow.cc

# 4. Header Files (Interfaces)
HEADERS += \
    src/core/include/Layer.h \
    src/core/include/LayerManager.h \
    src/core/include/LMObserver.h \
    src/core/include/Shape.h \
    src/gui/include/Canvas.h \
    src/gui/include/MainWindow.h