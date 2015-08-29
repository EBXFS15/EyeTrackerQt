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
LIBS += -L~/Downloads/OpenCV_2_4_10/lib\
         -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_video -lopencv_imgcodecs\
         -lx264 -liconv -lusc -lavcodec -lavformat -lavutil -lswscale -lopencv_videoio\
        -L/opt/embedded/bbb/rootfs/usr/lib\
         -ljpeg -lm -lz\
         -L/opt/embedded/bbb/rootfs/usr/local/lib\
         -lts\

INCLUDEPATH += /home/dpa/Downloads/OpenCV_2_4_10/include\
                /home/dpa/Downloads/OpenCV_2_4_10/include/opencv\
                /home/dpa/Downloads/OpenCV_2_4_10/include/opencv2\
#INCLUDEPATH += /opt/embedded/bbb/rootfs/usr/include/\
#                /opt/embedded/bbb/rootfs/usr/local/include\
#                /opt/embedded/bbb/rootfs/usr/include/opencv\
#                /opt/embedded/bbb/rootfs/usr/include/opencv2\

SOURCES += main.cpp\
        eyetrackerwindow.cpp

HEADERS  += eyetrackerwindow.h

FORMS    += eyetrackerwindow.ui
