#-------------------------------------------------
#
# Project created by QtCreator 2015-08-29T08:20:56
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = EyeTracker
target.path = /home/debian
INSTALLS += target

TEMPLATE = app
LIBS +=  -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs\
         -lx264 -liconv -lusc -lavcodec -lavformat -lavutil -lswscale -lopencv_videoio\
         -L/opt/embedded/bbb/rootfs/usr/lib\
         -ljpeg -lm -lz\
         -L/opt/embedded/bbb/rootfs/usr/local/lib\
         -lts\

INCLUDEPATH += /opt/embedded/bbb/rootfs/usr/include/\
                /opt/embedded/bbb/rootfs/usr/local/include\

SOURCES += main.cpp\
        eyetrackerwindow.cpp \
        v4l2camera.cpp \
    capturethread.cpp

HEADERS  += eyetrackerwindow.h \
    capturethread.h

FORMS    += eyetrackerwindow.ui

DISTFILES += \
    README.md
