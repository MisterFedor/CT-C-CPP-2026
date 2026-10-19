#pragma once

#include <QPushButton>

#define INIT_COORDINATES_VALUE -1
#define EMPTY_CELL 0

class SudokuCell : public QPushButton
{
    Q_OBJECT

public:
    explicit SudokuCell(int row, int col, QWidget *parent = nullptr);
    int get_row() const;
    int get_col() const;
    int get_value() const;
    bool set_value(int value);
    void highlight_givens(bool given);
    void highlight_error();
    void set_candidates(bool candidates[]);

signals:
    void clicked_cell(int row, int col);

private slots:
    void click_processing();
private:
    int row = INIT_COORDINATES_VALUE;
    int col = INIT_COORDINATES_VALUE;
    int cell_value = EMPTY_CELL;
};
