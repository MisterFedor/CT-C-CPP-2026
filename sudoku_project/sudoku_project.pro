QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++20

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    number_button.cpp \
    number_pad.cpp \
    sudoku_board.cpp \
    sudoku_cell.cpp \
    sudoku_file.cpp

HEADERS += \
    mainwindow.hpp \
    number_button.hpp \
    number_pad.hpp \
    sudoku_board.hpp \
    sudoku_cell.hpp \
    sudoku_file.hpp \
    sudoku_move.hpp

