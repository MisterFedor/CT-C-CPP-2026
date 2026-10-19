#pragma once

#include "sudoku_board.hpp"
#include "sudoku_move.hpp"

#include <QMainWindow>
#include <QVector>

#define NET_SIZE 9
#define INIT_TIME 0

class QFrame;
class QWidget;
class QGridLayout;
class SudokuCell;
class QHBoxLayout;
class QVBoxLayout;
class NumberPad;
class QTimer;
class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
private:
    void make_window();
    void make_net();
    void make_menu();
    void update_highlights();
    void update_time_status();
    void update_cell(int row, int col);

    QWidget *central_widget = nullptr;
    QWidget *control_widget = nullptr;
    QFrame *net_widget = nullptr;
    QHBoxLayout *main_layout = nullptr;
    QVBoxLayout *menu_layout = nullptr;
    QGridLayout *grid_layout = nullptr;

    SudokuCell *selected_cell = nullptr;
    SudokuCell *sudoku_net[NET_SIZE][NET_SIZE] = {{nullptr}};
    NumberPad *number_pad = nullptr;
    SudokuBoard sudoku_board;
    QVector<Move> user_moves;

    int game_time = INIT_TIME;
    QTimer *timer = nullptr;
    bool candidates_mode = false;
    QPushButton *candidates_button = nullptr;

private slots:
    void select_cell(int row, int col);
    void set_cell_value(int value);
    void load_game();
    void save_game();
    void undo_move();
    void increase_time();
    void switch_candidates_mode();
};
