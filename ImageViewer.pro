QT       += core widgets
QT       += multimedia
QT       += multimediawidgets
QT       += widgets
QT       += sql

CONFIG += c++17 cmdline

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

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

TRANSLATIONS += \
    ImageViewer_en_US.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

FORMS += \
    camerascreens.ui \
    camerasettings.ui \
    faceshandler.ui \
    focusview.ui \
    mainwindow.ui \
    rewindui.ui

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


# For MSVC Build
# win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../OpenCV/opencv/build/x64/vc16/lib/ -lopencv_world480
# else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../OpenCV/opencv/build/x64/vc16/lib/ -lopencv_world480d
# else:unix: LIBS += -L$$PWD/../OpenCV/opencv/build/x64/vc16/lib/ -lopencv_world480

# win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../OpenCV/opencv/build/install/x64/vc17/lib/ -lopencv_face480
# else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../OpenCV/opencv/build/install/x64/vc17/lib/ -lopencv_face480d
# else:unix: LIBS += -L$$PWD/../OpenCV/opencv/build/install/x64/vc17/lib/ -lopencv_face480

# INCLUDEPATH += $$PWD/../OpenCV/opencv/build/install/include
# DEPENDPATH += $$PWD/../OpenCV/opencv/build/install/include


# For MinGW Build
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../OpenCV-MinGW/build/install/x64/mingw/lib/ -llibopencv_world4100.dll
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../OpenCV-MinGW/build/install/x64/mingw/lib/ -llibopencv_world4100.dll
else:unix: LIBS += -L$$PWD/../OpenCV-MinGW/build/install/x64/mingw/lib/ -llibopencv_world4100.dll.a

INCLUDEPATH += $$PWD/../OpenCV-MinGW/build/install/include
DEPENDPATH += $$PWD/../OpenCV-MinGW/build/install/include

INCLUDEPATH += $$PWD/../dlib-19.24/include
DEPENDPATH += $$PWD/../dlib-19.24/include
LIBS += -L$$PWD/../dlib-19.24/build
LIBS += -ldlib

DISTFILES += \
    error.png

