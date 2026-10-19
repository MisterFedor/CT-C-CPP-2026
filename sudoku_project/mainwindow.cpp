#include "mainwindow.hpp"
#include "sudoku_cell.hpp"
#include "number_pad.hpp"
#include "sudoku_file.hpp"

#include <QWidget>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSizePolicy>
#include <QFrame>
#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <QTimer>
#include <QStatusBar>

#define INIT_WINDOW_WIDTH 1400
#define INIT_WINDOW_HEIGHT 1200
#define MIN_WINDOW_WIDTH 1100
#define MIN_WINDOW_HEIGHT 900
#define MAIN_BOX_LINE_WIDTH 4
#define SIZE_BETWEEN_BOXES 0
#define SIZE_BETWEEN_CELLS 0
#define BOXES_INDENT 0
#define NET_PARTS 3
#define MENU_PARTS 1
#define BOXES_AMOUNT 3
#define BOX_LINE_WIDTH 3
#define SMALL_BOXES_INDENT 0
#define BUTTON_MIN_WIDTH 250
#define BUTTON_MIN_HEIGHT 80
#define BUTTONS_INDENT 25
#define SIZE_BETWEEN_BUTTONS 20
#define STRETCH_SIZE 1
#define TIMER_START 1000
#define SECONDS_IN_HOUR 3600
#define SECONDS_IN_MINUTE 60
#define TWO_DIGIT_NUMBER 10
#define EMPTY_VALUE 0


MainWindow::MainWindow(QWidget *par)
    : QMainWindow(par)
{
    make_window();
    make_net();
    make_menu();

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::increase_time);
    update_time_status();
}

void MainWindow::make_window(){
    setWindowTitle("Sudoku Game");
    resize(INIT_WINDOW_WIDTH,INIT_WINDOW_HEIGHT);
    setMinimumSize(MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT);

    central_widget = new QWidget(this);
    setCentralWidget(central_widget);

    main_layout = new QHBoxLayout(central_widget);
    net_widget = new QFrame(central_widget);
    net_widget->setFrameShape(QFrame::Box);
    net_widget->setLineWidth(MAIN_BOX_LINE_WIDTH);
    grid_layout = new QGridLayout(net_widget);

    control_widget = new QWidget(central_widget);
    menu_layout = new QVBoxLayout(control_widget);

    grid_layout->setSpacing(SIZE_BETWEEN_BOXES);
    grid_layout->setContentsMargins(BOXES_INDENT, BOXES_INDENT, BOXES_INDENT, BOXES_INDENT);
    for (int i = 0; i < BOXES_AMOUNT; i++) {
        grid_layout->setRowStretch(i, STRETCH_SIZE);
        grid_layout->setColumnStretch(i, STRETCH_SIZE);
    }
    main_layout->addWidget(net_widget, NET_PARTS);
    main_layout->addWidget(control_widget, MENU_PARTS);

}

void MainWindow::make_net(){
    for (int boxes_row = 0; boxes_row < BOXES_AMOUNT; boxes_row++) {
          for (int boxes_col = 0; boxes_col < BOXES_AMOUNT; boxes_col++) {
              QFrame *box_frame = new QFrame(net_widget);
              box_frame->setFrameShape(QFrame::Box);
              box_frame->setLineWidth(BOX_LINE_WIDTH);

              QGridLayout *box_layout = new QGridLayout(box_frame);
              box_layout->setSpacing(SIZE_BETWEEN_CELLS);
              box_layout->setContentsMargins(SMALL_BOXES_INDENT, SMALL_BOXES_INDENT, SMALL_BOXES_INDENT, SMALL_BOXES_INDENT);
              for (int i = 0; i < BOXES_AMOUNT; i++) {
                  box_layout->setRowStretch(i, STRETCH_SIZE);
                  box_layout->setColumnStretch(i, STRETCH_SIZE);
              }
              grid_layout->addWidget(box_frame, boxes_row, boxes_col);

              for (int box_row = 0; box_row < BOXES_AMOUNT; box_row++) {
                  for (int box_col = 0; box_col < BOXES_AMOUNT; box_col++) {
                      int row = boxes_row * BOXES_AMOUNT + box_row;
                      int col = boxes_col * BOXES_AMOUNT + box_col;
                      sudoku_net[row][col] = new SudokuCell(row, col, box_frame);
                      box_layout->addWidget(sudoku_net[row][col], box_row, box_col);
                      connect(sudoku_net[row][col], &SudokuCell::clicked_cell, this, &MainWindow::select_cell);
                  }
              }
          }
      }
}

void MainWindow::make_menu(){
    menu_layout->addStretch();
    number_pad = new NumberPad(control_widget);
    menu_layout->addWidget(number_pad);
    connect(number_pad, &NumberPad::clicked_number, this, &MainWindow::set_cell_value);
    menu_layout->addStretch();

    QPushButton *load_button = new QPushButton("Load net with givens", control_widget);
    QPushButton *save_button = new QPushButton("Save game", control_widget);
    QPushButton *undo_button = new QPushButton("Undo move", control_widget);
    candidates_button = new QPushButton("Candidates mode: OFF", control_widget);
    QPushButton *strategy_button = new QPushButton("Make move with strategy", control_widget);

    QPushButton *buttons[] = {
        load_button,
        save_button,
        undo_button,
        candidates_button,
        strategy_button
    };

    connect(load_button, &QPushButton::clicked, this, &MainWindow::load_game);
    connect(save_button, &QPushButton::clicked, this, &MainWindow::save_game);
    connect(undo_button, &QPushButton::clicked, this, &MainWindow::undo_move);
    connect(candidates_button, &QPushButton::clicked, this, &MainWindow::switch_candidates_mode);

    menu_layout->setContentsMargins(BUTTONS_INDENT, 0, BUTTONS_INDENT, 0);
    menu_layout->setSpacing(SIZE_BETWEEN_BUTTONS);

    for (QPushButton *button : buttons) {
        button->setMinimumSize(BUTTON_MIN_WIDTH, BUTTON_MIN_HEIGHT);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        menu_layout->addWidget(button);
    }
    menu_layout->addStretch();
}

void MainWindow::select_cell(int row, int col)
{
    if (selected_cell != nullptr) {
        selected_cell->setChecked(false);
    }
    selected_cell = sudoku_net[row][col];
    selected_cell->setChecked(true);
}

void MainWindow::set_cell_value(int value){
    if (selected_cell == nullptr){
            return;
    }

    int row = selected_cell->get_row();
    int col = selected_cell->get_col();
    int prev_value = sudoku_board.get_value(row, col);

    if (candidates_mode == true) {
        if (sudoku_board.show_candidate(row, col, value) == true) {
            update_cell(row, col);

            if (timer->isActive() == false) {
                timer->start(TIMER_START);
            }
        }
        return;
    }

    if (prev_value == value) {
        return;
    }

    if (sudoku_board.set_value(row, col, value) == false) {
        return;
    }

    if (timer->isActive() == false) {
        timer->start(TIMER_START);
    }

    Move move;
    move.row = row;
    move.col = col;
    move.prev_value = prev_value;
    move.new_value = value;
    user_moves.push_back(move);

    update_cell(row, col);
    update_highlights();

    if (sudoku_board.is_end() == true) {
        timer->stop();
        QMessageBox::information(this, "Game end", "Sudoku solved!");
    }
}

void MainWindow::load_game()
{
    QString file_name = QFileDialog::getOpenFileName(this, "Choose file");
    if (SudokuFile::load_game(file_name, sudoku_board, user_moves, game_time) == false) {
        QMessageBox::warning(this, "Load error", "Can`t load file. Check format.");
        return;
    }

    for (int row = 0; row < NET_SIZE; row++) {
        for (int col = 0; col < NET_SIZE; col++) {
            update_cell(row, col);
        }
    }

    timer->stop();
    update_time_status();
    update_highlights();

    if (sudoku_board.is_end() == true) {
        QMessageBox::information(this, "Game end", "Sudoku solved!");
    }
}

void MainWindow::save_game()
{
    QString file_name = QFileDialog::getSaveFileName(this, "Save game");
    if (SudokuFile::save_game(file_name, sudoku_board, user_moves, game_time) == false) {
        QMessageBox::warning(this, "Save error", "Can`t save file.");
        return;
    }
}

void MainWindow::update_highlights()
{
    for (int row = 0; row < NET_SIZE; row++) {
        for (int col = 0; col < NET_SIZE; col++) {
            if (sudoku_board.has_error(row, col) == true) {
                sudoku_net[row][col]->highlight_error();
            } else {
                sudoku_net[row][col]->highlight_givens(sudoku_board.is_given(row, col));
            }
        }
    }
}

void MainWindow::update_cell(int row, int col)
{
    int value = sudoku_board.get_value(row, col);

    if (value != EMPTY_VALUE) {
        sudoku_net[row][col]->set_value(value);
        return;
    }

    bool candidates[CANDIDATES_AMOUNT] = {false};

    for (int candidate = 1; candidate < CANDIDATES_AMOUNT; candidate++) {
        candidates[candidate] = sudoku_board.has_candidate(row, col, candidate);
    }

    sudoku_net[row][col]->set_candidates(candidates);
}

void MainWindow::undo_move()
{
    if (user_moves.isEmpty() == true) {
        return;
    }

    Move move = user_moves.back();
    user_moves.pop_back();

    sudoku_board.set_value(move.row, move.col, move.prev_value);
    update_cell(move.row, move.col);

    update_highlights();
}

void MainWindow::update_time_status()
{
    int hours = game_time / SECONDS_IN_HOUR;
    int minutes = (game_time / SECONDS_IN_MINUTE) % SECONDS_IN_MINUTE;
    int seconds = game_time % SECONDS_IN_MINUTE;

    QString hours_str = QString::number(hours);
    QString minutes_str = QString::number(minutes);
    QString seconds_str = QString::number(seconds);

    if (hours < TWO_DIGIT_NUMBER) {
        hours_str = "0" + hours_str;
    }
    if (minutes < TWO_DIGIT_NUMBER) {
        minutes_str = "0" + minutes_str;
    }
    if (seconds < TWO_DIGIT_NUMBER) {
        seconds_str = "0" + seconds_str;
    }

    QString status_timer = "Solve time: ";
    status_timer += hours_str;
    status_timer += ":";
    status_timer += minutes_str;
    status_timer += ":";
    status_timer += seconds_str;

    statusBar()->showMessage(status_timer);
}

void MainWindow::increase_time()
{
    game_time++;
    update_time_status();
}

void MainWindow::switch_candidates_mode()
{
    if (candidates_mode == true) {
        candidates_mode = false;
        candidates_button->setText("Candidates mode: OFF");
    } else {
        candidates_mode = true;
        candidates_button->setText("Candidates mode: ON");
    }
}
