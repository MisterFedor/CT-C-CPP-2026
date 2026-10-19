#pragma once

#include "sudoku_move.hpp"

#include <QVector>

class QString;
class SudokuBoard;

class SudokuFile
{
public:
    static bool load_game(const QString &file_name, SudokuBoard &sudoku_board, QVector<Move> &user_moves, int &game_time);
    static bool save_game(const QString &file_name, SudokuBoard &sudoku_board, QVector<Move> &user_moves, int game_time);
};
