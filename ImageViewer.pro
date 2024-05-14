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
        customlabel.cpp \
        facerecognition.cpp \
        focusview.cpp \
        loginform.cpp \
        main.cpp \
        mainwindow.cpp \
        recordingworker.cpp \
        rewindui.cpp \
        singleviewwidget.cpp

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
    focusview.ui \
    loginform.ui \
    mainwindow.ui \
    rewindui.ui \
    singleviewwidget.ui

HEADERS += \
    camerahandler.h \
    camerascreens.h \
    camerasettings.h \
    customlabel.h \
    facerecognition.h \
    focusview.h \
    loginform.h \
    mainwindow.h \
    recordingworker.h \
    rewindui.h \
    singleviewwidget.h


win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../OpenCV/opencv/build/x64/vc16/lib/ -lopencv_world480
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../OpenCV/opencv/build/x64/vc16/lib/ -lopencv_world480d
else:unix: LIBS += -L$$PWD/../OpenCV/opencv/build/x64/vc16/lib/ -lopencv_world480

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../OpenCV/opencv/build/install/x64/vc17/lib/ -lopencv_face480
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../OpenCV/opencv/build/install/x64/vc17/lib/ -lopencv_face480d
else:unix: LIBS += -L$$PWD/../OpenCV/opencv/build/install/x64/vc17/lib/ -lopencv_face480



INCLUDEPATH += $$PWD/../OpenCV/opencv/build/install/include
DEPENDPATH += $$PWD/../OpenCV/opencv/build/install/include

DISTFILES += \
    error.png
