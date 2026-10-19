#include "number_button.hpp"

#include <QString>

NumberButton::NumberButton(int value, QWidget *parent)
    : QPushButton(QString::number(value), parent), value(value)
{
    connect(this, &QPushButton::clicked, this, &NumberButton::click_processing);
}

void NumberButton::click_processing()
{
    emit clicked_number(value);
}
