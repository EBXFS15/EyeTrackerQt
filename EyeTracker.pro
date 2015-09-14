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
LIBS +=  -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_objdetect -lopencv_imgcodecs\
         -lx264 -liconv -lusc -lavcodec -lavformat -lavutil -lswscale -lopencv_videoio\
         -lv4l1 -lv4l2 -lm -lz\
         -L/opt/embedded/bbb/rootfs/usr/lib\
         -L/opt/embedded/bbb/rootfs/usr/local/lib\
         -L/opt/embedded/bbb/rootfs/usr/lib/arm-linux-gnueabihf\

QMAKE_RPATHDIR += /opt/embedded/bbb/rootfs/usr/lib/arm-linux-gnueabihf\

INCLUDEPATH += /opt/embedded/bbb/rootfs/usr/include/\
               /opt/embedded/bbb/rootfs/usr/local/include\
               #/opt/embedded/bbb/rootfs/opt/cr

SOURCES += main.cpp\
        eyetrackerwindow.cpp \
        v4l2camera.cpp \
    captureWorker.cpp \
    eyetrackerWorker.cpp \
    ebxMonitorWorker.cpp

HEADERS  += eyetrackerwindow.h \
    captureWorker.h \
    eyetrackerWorker.h \
    ebxMonitorWorker.h

FORMS    += eyetrackerwindow.ui

DISTFILES += \
    README.md
