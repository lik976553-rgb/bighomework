QT += widgets network

TEMPLATE = app
TARGET = bighomework

CONFIG += c++17

msvc:QMAKE_CXXFLAGS += /utf-8
win32-g++:QMAKE_CXXFLAGS += -finput-charset=UTF-8 -fexec-charset=UTF-8

SOURCES += main.cpp

HEADERS += \
    ai_parser.h \
    deepseek_config.h \
    diet_manager.h \
    diet_system.h \
    food.h \
    food_factory.h \
    nutrition_db.h \
    record_manager.h \
    user_manager.h

DISTFILES += nutrition_db.csv

DEFINES += NUTRITION_DB_PATH=\\\"$$PWD/nutrition_db.csv\\\"
