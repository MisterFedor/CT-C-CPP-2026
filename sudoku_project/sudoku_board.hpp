#pragma once

#define BOARD_SIZE 9
#define CANDIDATES_AMOUNT 10

class SudokuBoard
{
public:
    SudokuBoard();
    void clear_board();
    bool set_value(int row, int col, int value);
    bool set_given(int row, int col, int value);

    int get_value(int row, int col) const;
    bool is_given(int row, int col) const;

    bool is_correct() const;
    bool is_end() const;
    bool has_error(int row, int col) const;

    bool show_candidate(int row, int col, int value);
    bool has_candidate(int row, int col, int value) const;
    void fill_candidates();

private:
    int values[BOARD_SIZE][BOARD_SIZE];
    bool givens[BOARD_SIZE][BOARD_SIZE];
    bool candidates[BOARD_SIZE][BOARD_SIZE][CANDIDATES_AMOUNT];
};
