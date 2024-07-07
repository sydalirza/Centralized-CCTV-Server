QT       += core widgets multimedia multimediawidgets sql

CONFIG += c++17 cmdline

# Disable deprecated APIs before Qt 6.0.0
# DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

# Sources
SOURCES += \
    camerahandler.cpp \
    camerascreens.cpp \
    camerasettings.cpp \
    cameraworker.cpp \
    customlabel.cpp \
    dlib_utils.cpp \
    faceshandler.cpp \
    focusview.cpp \
    main.cpp \
    mainwindow.cpp \
    recordingworker.cpp \
    rewindui.cpp

# Headers
HEADERS += \
    camerahandler.h \
    camerascreens.h \
    camerasettings.h \
    cameraworker.h \
    customlabel.h \
    dlib_utils.h \
    faceshandler.h \
    focusview.h \
    mainwindow.h \
    recordingworker.h \
    rewindui.h

# Forms
FORMS += \
    camerascreens.ui \
    camerasettings.ui \
    faceshandler.ui \
    focusview.ui \
    mainwindow.ui \
    rewindui.ui

# Translations
TRANSLATIONS += \
    ImageViewer_en_US.ts

CONFIG += lrelease
CONFIG += embed_translations

# Deployment rules
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Libraries and Include Paths
win32 {
    CONFIG(release, debug|release): {
        LIBS += -L$$PWD/../OpenCV-MinGW/build/install/x64/mingw/lib/ -llibopencv_world4100.dll
        LIBS += -L$$PWD/../dlib-19.24/build -ldlib
    }
    CONFIG(debug, debug|release): {
        LIBS += -L$$PWD/../OpenCV-MinGW/build/install/x64/mingw/lib/ -llibopencv_world4100.dll
        LIBS += -L$$PWD/../dlib-19.24/build -ldlib
    }
    INCLUDEPATH += $$PWD/../OpenCV-MinGW\build\install\include
    DEPENDPATH += $$PWD/../OpenCV-MinGW\build\install\include

    INCLUDEPATH += $$PWD/../dlib-19.24/include
    DEPENDPATH += $$PWD/../dlib-19.24/include
} else: unix {
    LIBS += -L/Programs/OpenCV/build/lib -lopencv_world
    LIBS += -L/Programs/dlib-19.24/build -ldlib

    INCLUDEPATH += /usr/local/include/opencv4
    DEPENDPATH += /usr/local/include/opencv4

    INCLUDEPATH += /usr/local/include
    DEPENDPATH += /usr/local/include
}

# Additional resources or files to be included in the distribution package
DISTFILES += \
    error.png
