#-------------------------------------------------
#
# Project created by QtCreator 2017-05-16T10:41:54
#
#-------------------------------------------------

QT       += core gui webenginewidgets network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QImageScraper
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += ..

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += main.cpp\
        mainwindow.cpp \
    core/bing_image_search.cpp \
    core/image_search.cpp

HEADERS  += mainwindow.hpp \
    core/bing_image_search.hpp \
    core/image_search.hpp

FORMS    += mainwindow.ui