QT += \
core\
gui\
multimedia\
multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += $$PWD/include
INCLUDEPATH += $$PWD/include/kvazaar

LIBS += -L$$PWD/libs
LIBS += -luvgrtp
LIBS += -lkvazaar

LIBS += -lws2_32
LIBS += -lpthread

SOURCES += \
    convertcolorspace.cpp \
    main.cpp \
    mainwindow.cpp \
    videoencoder.cpp \
    videoinputstream.cpp \
    videosurface.cpp

HEADERS += \
    convertcolorspace.h \
    mainwindow.h \
    videoencoder.h \
    videoinputstream.h \
    videosurface.h
    ui_mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
