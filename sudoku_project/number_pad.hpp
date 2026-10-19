#pragma once

#include <QWidget>
#define NUMBER_BUTTONS_AMOUNT 9

class QGridLayout;
class NumberButton;
class QPushButton;

class NumberPad : public QWidget
{
    Q_OBJECT

public:
    explicit NumberPad(QWidget *parent = nullptr);

signals:
    void clicked_number(int value);
private slots:
    void clear_number();
private:
    void make_number_buttons();
    void make_clear_button();
    void keyboard_input();

    QGridLayout *grid_layout = nullptr;
    QPushButton *clear_button = nullptr;
    NumberButton *buttons[NUMBER_BUTTONS_AMOUNT] = {nullptr};
};
