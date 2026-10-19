#include "sudoku_board.hpp"

#define EMPTY_CELL_VALUE 0
#define MIN_CELL_VALUE 0
#define MAX_CELL_VALUE 9
#define BOX_SIZE 3
#define CANDIDATES_AMOUNT 10

static bool check_value(int row, int col, int value)
{
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        return false;
    }
    if (value < MIN_CELL_VALUE || value > MAX_CELL_VALUE) {
        return false;
    }

    return true;
}

SudokuBoard::SudokuBoard()
{
    clear_board();
}

void SudokuBoard::clear_board()
{
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            values[row][col] = EMPTY_CELL_VALUE;
            givens[row][col] = false;
            for (int value = 0; value < CANDIDATES_AMOUNT; value++) {
                candidates[row][col][value] = false;
            }
        }
    }
}

bool SudokuBoard::set_value(int row, int col, int value)
{
    if (check_value(row, col, value) == false) {
        return false;
    }
    if (givens[row][col] == true) {
        return false;
    }

    values[row][col] = value;
    for (int i = 0; i < CANDIDATES_AMOUNT; i++) {
        candidates[row][col][i] = false;
    }
    return true;
}

bool SudokuBoard::set_given(int row, int col, int value)
{
    if (check_value(row, col, value) == false) {
        return false;
    }
    if (value == EMPTY_CELL_VALUE) {
        return false;
    }

    values[row][col] = value;
    givens[row][col] = true;
    for (int i = 0; i < CANDIDATES_AMOUNT; i++) {
        candidates[row][col][i] = false;
    }
    return true;
}

int SudokuBoard::get_value(int row, int col) const
{
    if (check_value(row, col, EMPTY_CELL_VALUE) == false) {
        return EMPTY_CELL_VALUE;
    }

    return values[row][col];
}

bool SudokuBoard::is_given(int row, int col) const
{
    if (check_value(row, col, EMPTY_CELL_VALUE) == false) {
        return false;
    }

    return givens[row][col];
}

bool SudokuBoard::is_correct() const
{
    for (int row = 0; row < BOARD_SIZE; row++) {
        bool used[MAX_CELL_VALUE + 1] = {false};

        for (int col = 0; col < BOARD_SIZE; col++) {
            int value = values[row][col];

            if (value == EMPTY_CELL_VALUE) {
                continue;
            }
            if (used[value] == true) {
                return false;
            }

            used[value] = true;
        }
    }

    for (int col = 0; col < BOARD_SIZE; col++) {
        bool used[MAX_CELL_VALUE + 1] = {false};

        for (int row = 0; row < BOARD_SIZE; row++) {
            int value = values[row][col];

            if (value == EMPTY_CELL_VALUE) {
                continue;
            }
            if (used[value] == true) {
                return false;
            }

            used[value] = true;
        }
    }

    for (int box_row = 0; box_row < BOARD_SIZE; box_row += BOX_SIZE) {
        for (int box_col = 0; box_col < BOARD_SIZE; box_col += BOX_SIZE) {
            bool used[MAX_CELL_VALUE + 1] = {false};

            for (int row = box_row; row < box_row + BOX_SIZE; row++) {
                for (int col = box_col; col < box_col + BOX_SIZE; col++) {
                    int value = values[row][col];

                    if (value == EMPTY_CELL_VALUE) {
                        continue;
                    }
                    if (used[value] == true) {
                        return false;
                    }

                    used[value] = true;
                }
            }
        }
    }

    return true;
}

bool SudokuBoard::is_end() const
{
    if (is_correct() == false) {
        return false;
    }

    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            if (values[row][col] == EMPTY_CELL_VALUE) {
                return false;
            }
        }
    }

    return true;
}

bool SudokuBoard::has_error(int row, int col) const
{
    if (check_value(row, col, EMPTY_CELL_VALUE) == false) {
        return false;
    }

    int value = values[row][col];

    if (value == EMPTY_CELL_VALUE) {
        return false;
    }

    for (int i = 0; i < BOARD_SIZE; i++) {
        if (i != col && values[row][i] == value) {
            return true;
        }
        if (i != row && values[i][col] == value) {
            return true;
        }
    }

    int box_row = row / BOX_SIZE * BOX_SIZE;
    int box_col = col / BOX_SIZE * BOX_SIZE;

    for (int i = box_row; i < box_row + BOX_SIZE; i++) {
        for (int j = box_col; j < box_col + BOX_SIZE; j++) {
            if (i == row && j == col) {
                continue;
            }
            if (values[i][j] == value) {
                return true;
            }
        }
    }
    return false;
}

bool SudokuBoard::show_candidate(int row, int col, int value)
{
    if (check_value(row, col, value) == false) {
        return false;
    }

    if (values[row][col] != EMPTY_CELL_VALUE) {
        return false;
    }

    if (value == EMPTY_CELL_VALUE) {
        for (int i = 0; i < CANDIDATES_AMOUNT; i++) {
            candidates[row][col][i] = false;
        }
        return true;
    }

    if (candidates[row][col][value] == true) {
        candidates[row][col][value] = false;
    } else {
        candidates[row][col][value] = true;
    }
    return true;
}

bool SudokuBoard::has_candidate(int row, int col, int value) const
{
    if (check_value(row, col, value) == false) {
        return false;
    }
    if (value == EMPTY_CELL_VALUE) {
        return false;
    }

    return candidates[row][col][value];
}

void SudokuBoard::fill_candidates()
{
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            for (int value = 0; value < CANDIDATES_AMOUNT; value++) {
                candidates[row][col][value] = false;
            }

            if (values[row][col] != EMPTY_CELL_VALUE) {
                continue;
            }

            for (int value = 1; value <= MAX_CELL_VALUE; value++) {
                bool can_be_candidate = true;

                for (int i = 0; i < BOARD_SIZE; i++) {
                    if (values[row][i] == value) {
                        can_be_candidate = false;
                    }
                    if (values[i][col] == value) {
                        can_be_candidate = false;
                    }
                }

                int box_row = row / BOX_SIZE * BOX_SIZE;
                int box_col = col / BOX_SIZE * BOX_SIZE;

                for (int i = box_row; i < box_row + BOX_SIZE; i++) {
                    for (int j = box_col; j < box_col + BOX_SIZE; j++) {
                        if (values[i][j] == value) {
                            can_be_candidate = false;
                        }
                    }
                }

                candidates[row][col][value] = can_be_candidate;
            }
        }
    }
}
