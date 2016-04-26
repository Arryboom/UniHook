#-------------------------------------------------
#
# Project created by QtCreator 2016-03-14T18:00:08
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = UniHookUI
TEMPLATE = app


SOURCES += main.cpp\
        MainWindow.cpp \
    DlgProcessList.cpp \
    DlgMemory.cpp \
    DlgFind.cpp

HEADERS  += MainWindow.h \
    DlgProcessList.h \
    DlgMemory.h \
    Regions.h \
    DlgFind.h \
    ../Common/IPC/SharedMemQueue.h \
    ../Common/IPC/SharedMemMutex.h \
    injector.h \
    dlgregisterview.h

FORMS    += MainWindow.ui \
    DlgProcessList.ui \
    DlgMemory.ui \
    DlgFind.ui \
    DlgViewRegister.ui

RESOURCES += \
    Resource.qrc

win32:RC_ICONS += images\small.ico
