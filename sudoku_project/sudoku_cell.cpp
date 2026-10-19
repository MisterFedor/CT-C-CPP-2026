#include "sudoku_cell.hpp"
#include <QString>
#include <QSizePolicy>

#define INIT_CELL_VALUE 0
#define CELL_MIN_SIZE 60
#define MAX_CELL_VALUE 9
#define MIN_CELL_VALUE 0
#define EMPTY_CELL 0
#define GIVEN_CELL_STYLE "background-color: rgb(135, 206, 250);"
#define NORMAL_CELL_STYLE "background-color: rgb(255, 245, 238);"
#define ERROR_CELL_STYLE "background-color: rgb(255, 160, 160);"

SudokuCell::SudokuCell(int r, int c, QWidget *par)
    : QPushButton(par), row(r), col(c)
{
    setMinimumSize(CELL_MIN_SIZE,CELL_MIN_SIZE);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    set_value(INIT_CELL_VALUE);
    highlight_givens(false);
    setCheckable(true);
    connect(this, &QPushButton::clicked, this, &SudokuCell::click_processing);
}

int SudokuCell::get_row() const{
    return row;
}

int SudokuCell::get_col() const{
    return col;
}

int SudokuCell::get_value() const{
    return cell_value;
}

bool SudokuCell::set_value(int value){
    if (value < MIN_CELL_VALUE || value > MAX_CELL_VALUE){
        return false;
    }
    cell_value = value;
    if (cell_value == EMPTY_CELL){
        setText("");
    } else {
        QString str_value = QString::number(cell_value);
        setText(str_value);
    }
    return true;
}

void SudokuCell::highlight_givens(bool given)
{
    if (given == true) {
        setStyleSheet(GIVEN_CELL_STYLE);
    } else {
        setStyleSheet(NORMAL_CELL_STYLE);
    }
}

void SudokuCell::highlight_error()
{
    setStyleSheet(ERROR_CELL_STYLE);
}

void SudokuCell::set_candidates(bool candidates[])
{
    QString cell_candidates = "";

    for (int value = 1; value <= MAX_CELL_VALUE; value++) {
        if (candidates[value] == true) {
            cell_candidates += QString::number(value);
            cell_candidates += " ";
        }

        if (value == 3 || value == 6) {
            cell_candidates += "\n";
        }
    }

    cell_value = EMPTY_CELL;
    setText(cell_candidates);
}

void SudokuCell::click_processing()
{
    emit clicked_cell(row, col);
}
