#-------------------------------------------------
#
# Project created by QtCreator 2013-04-29T18:07:01
#
#-------------------------------------------------

QT += core opengl

TARGET = volume
TEMPLATE = app

HEADERS += \
    Widgets/LutWidget.h \
    Widgets/SliceWidget.h \
    ui/MainWindow.h \
    ui/OpenWizard.h \
    common.h \
    Volume.h \
    LightSource.h \
    Formats/Loader.h \
    Formats/DDSLoader.h \
    Formats/RawLoader.h \
    Widgets/VolRenderer.h

SOURCES += main.cpp \
    Widgets/LutWidget.cpp \
    Widgets/SliceWidget.cpp \
    ui/MainWindow.cpp \
    ui/OpenWizard.cpp \
    common.cpp \
    Volume.cpp \
    LightSource.cpp \
    Formats/DDSLoader.cpp \
    Formats/RawLoader.cpp \
    Formats/Loader.cpp \
    Widgets/VolRenderer.cpp

FORMS += \
    ui/MainWindow.ui \
    ui/OpenWizard.ui

OTHER_FILES += \
    data/shader/directions.frag \
    data/shader/directions.vert \
    data/shader/raycast.frag \
    data/shader/raycast.vert

win32 {
    SOURCES += windows_compat.cpp
    HEADERS += windows_compat.h windows_compat_glext.h
}

DEFINES += USE_DICOM
CONFIG += USE_DICOM

USE_DICOM {
    SOURCES += Formats/DicomLoader.cpp \
        $$files(lib/base/src/*.cpp) \
        $$files(lib/imebra/src/*.cpp)

    HEADERS += Formats/DicomLoader.h \
        $$files(lib/base/include/*.h) \
        $$files(lib/imebra/include/*.h)
}

QMAKE_CXXFLAGS += -std=c++11 -Wall #-O3
