#include "number_pad.hpp"
#include "number_button.hpp"

#include <QGridLayout>
#include <QSizePolicy>
#include <QShortcut>
#include <QKeySequence>
#include <QString>
#include <QPushButton>

#define NUMBER_BUTTON_MIN_WIDTH 80
#define NUMBER_BUTTON_MIN_HEIGHT 80
#define CLEAR_BUTTON_MIN_WIDTH 80
#define CLEAR_BUTTON_MIN_HEIGHT 60
#define SIZE_BETWEEN_BUTTONS 5
#define NUMBER_PAD_SIZE 3
#define NUMBER_BUTTONS_AMOUNT 9
#define CLEAR_CELL_VALUE 0

NumberPad::NumberPad(QWidget *parent)
    : QWidget(parent)
{
    grid_layout = new QGridLayout(this);
    grid_layout->setSpacing(SIZE_BETWEEN_BUTTONS);
    grid_layout->setContentsMargins(0, 0, 0, 0);

    make_number_buttons();
    make_clear_button();
    keyboard_input();
}

void NumberPad::make_number_buttons()
{
    for (int i = 0; i < NUMBER_BUTTONS_AMOUNT; i++) {
        buttons[i] = new NumberButton((i + 1), this);
        buttons[i]->setMinimumSize(NUMBER_BUTTON_MIN_WIDTH, NUMBER_BUTTON_MIN_HEIGHT);
        buttons[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        int row = i / NUMBER_PAD_SIZE;
        int col = i % NUMBER_PAD_SIZE;

        grid_layout->addWidget(buttons[i], row, col);

        connect(buttons[i], &NumberButton::clicked_number,this, &NumberPad::clicked_number);
    }
}

void NumberPad::make_clear_button()
{
    clear_button = new QPushButton("Clear", this);
    clear_button->setMinimumSize(CLEAR_BUTTON_MIN_WIDTH, CLEAR_BUTTON_MIN_HEIGHT);
    clear_button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    grid_layout->addWidget(clear_button, NUMBER_PAD_SIZE, 0, 1, NUMBER_PAD_SIZE);


    connect(clear_button, &QPushButton::clicked, this, &NumberPad::clear_number);
}

void NumberPad::keyboard_input()
{
    for (int i = 0; i < NUMBER_BUTTONS_AMOUNT; i++) {
        QString str_value = QString::number(i + 1);
        QShortcut *number_key = new QShortcut(QKeySequence(str_value), this);
        number_key->setContext(Qt::WindowShortcut);
        connect(number_key, &QShortcut::activated, buttons[i], &NumberButton::click);
    }

    QShortcut *backspace_key = new QShortcut(QKeySequence(Qt::Key_Backspace), this);
    backspace_key->setContext(Qt::WindowShortcut);
    connect(backspace_key, &QShortcut::activated, this, &NumberPad::clear_number);
}

void NumberPad::clear_number()
{
    emit clicked_number(CLEAR_CELL_VALUE);
}
