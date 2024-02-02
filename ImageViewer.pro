QT       += core widgets
QT       += multimedia
QT       += multimediawidgets

CONFIG += c++17 cmdline

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        camerahandler.cpp \
        camerascreens.cpp \
        customlabel.cpp \
        main.cpp \
        mainwindow.cpp \
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
    mainwindow.ui \
    singleviewwidget.ui

HEADERS += \
    camerahandler.h \
    camerascreens.h \
    customlabel.h \
    mainwindow.h \
    singleviewwidget.h


win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../OpenCV/opencv/build/x64/vc16/lib/ -lopencv_world480
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../OpenCV/opencv/build/x64/vc16/lib/ -lopencv_world480d
else:unix: LIBS += -L$$PWD/../OpenCV/opencv/build/x64/vc16/lib/ -lopencv_world480

INCLUDEPATH += $$PWD/../OpenCV/opencv/build/include
DEPENDPATH += $$PWD/../OpenCV/opencv/build/include

DISTFILES += \
    error.png

