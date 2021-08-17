
QT       += core  network

TARGET = CQtHttpServer
TEMPLATE = app

SOURCES += \
    main.cpp

INCLUDEPATH += $$PWD/../httpserver
include (../Global.prf)

CONFIG(debug, debug|release){

    message("mess CQtHttpServer debug")

    LIBS += -lQtHttpServerd

}else{
    message("mess CQtHttpServer release")

    LIBS += -lQtHttpServer
}




