#include "sudoku_file.hpp"
#include "sudoku_board.hpp"
#include "sudoku_move.hpp"

#include <QFile>
#include <QTextStream>
#include <QIODevice>
#include <QString>
#include <QVector>

#define FILE_MARKER "MisterFedor sudoku"
#define GIVENS_MARKER "givens"
#define MOVES_MARKER "moves"
#define TIME_MARKER "time"

#define MIN_FILE_COORDINATE 1
#define MAX_FILE_COORDINATE 9
#define MIN_FILE_VALUE 0
#define MAX_FILE_VALUE 9
#define EMPTY_FILE_VALUE 0

static bool check_file_cell(int row, int col, int value)
{
    if (row < MIN_FILE_COORDINATE || row > MAX_FILE_COORDINATE || col < MIN_FILE_COORDINATE || col > MAX_FILE_COORDINATE ||
        value < MIN_FILE_VALUE || value > MAX_FILE_VALUE) {
        return false;
    }

    return true;
}

static bool read_givens(QTextStream &input, SudokuBoard &sudoku_board)
{
    QString givens_marker;
    int cnt;

    input >> givens_marker;

    if (givens_marker != GIVENS_MARKER) {
        return false;
    }

    input >> cnt;

    if (cnt < 0) {
        return false;
    }

    for (int i = 0; i < cnt; i++) {
        int row;
        int col;
        int value;

        input >> row >> col >> value;

        if (check_file_cell(row, col, value) == false) {
            return false;
        }
        if (value == EMPTY_FILE_VALUE) {
            return false;
        }
        if (sudoku_board.set_given(row - 1, col - 1, value) == false) {
            return false;
        }
    }

    return true;
}

static bool read_moves(QTextStream &input, SudokuBoard &sudoku_board, QVector<Move> &user_moves)
{
    QString moves_marker;
    int cnt;

    input >> moves_marker;

    if (moves_marker != MOVES_MARKER) {
        return false;
    }

    input >> cnt;

    if (cnt < 0) {
        return false;
    }

    for (int i = 0; i < cnt; i++) {
        Move move;

        input >> move.row >> move.col >> move.prev_value >> move.new_value;

        if (check_file_cell(move.row, move.col, move.prev_value) == false ||
            check_file_cell(move.row, move.col, move.new_value) == false) {
            return false;
        }

        move.row--;
        move.col--;

        if (sudoku_board.set_value(move.row, move.col, move.new_value) == false) {
            return false;
        }

        user_moves.push_back(move);
    }

    return true;
}

static bool read_time(QTextStream &input, int &game_time)
{
    QString time_marker;

    input >> time_marker;

    if (time_marker != TIME_MARKER) {
        return false;
    }

    input >> game_time;

    if (game_time < 0) {
        return false;
    }

    return true;
}

bool SudokuFile::load_game(const QString &filename, SudokuBoard &sudoku_board, QVector<Move> &user_moves, int &game_time)
{
    QFile file(filename);

    if (file.open(QIODevice::ReadOnly) == false) {
        return false;
    }

    QTextStream input(&file);

    QString sudoku_marker = input.readLine();

    if (sudoku_marker != FILE_MARKER) {
        return false;
    }

    SudokuBoard new_board;
    QVector<Move> new_moves;
    int new_time = 0;

    if (read_givens(input, new_board) == false) {
        return false;
    }

    if (new_board.is_correct() == false) {
        return false;
    }

    if (read_moves(input, new_board, new_moves) == false) {
        return false;
    }
    if (read_time(input, new_time) == false) {
        return false;
    }
    if (input.status() != QTextStream::Ok) {
        return false;
    }

    sudoku_board = new_board;
    user_moves = new_moves;
    game_time = new_time;

    return true;
}

bool SudokuFile::save_game(const QString &filename, SudokuBoard &sudoku_board, QVector<Move> &user_moves, int game_time)
{
    QFile file(filename);

    if (file.open(QIODevice::WriteOnly) == false) {
        return false;
    }

    QTextStream output(&file);

    output << FILE_MARKER << "\n";

    int givens_count = 0;

    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            if (sudoku_board.is_given(row, col) == true) {
                givens_count++;
            }
        }
    }

    output << GIVENS_MARKER << "\n";
    output << givens_count << "\n";

    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            if (sudoku_board.is_given(row, col) == true) {
                output << (row + 1) << " " << (col + 1) << " " << sudoku_board.get_value(row, col) << "\n";
            }
        }
    }

    output << MOVES_MARKER << "\n";
    output << user_moves.size() << "\n";

    for (int i = 0; i < user_moves.size(); i++) {
        Move move = user_moves[i];
        output << (move.row + 1) << " " << (move.col + 1) << " " << move.prev_value << " " << move.new_value << "\n";
    }
    output << TIME_MARKER << "\n";
    output << game_time << "\n";

    return true;
}
