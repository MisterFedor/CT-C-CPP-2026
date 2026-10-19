#pragma once

#include <QPushButton>
#define INIT_VALUE 0

class NumberButton : public QPushButton
{
    Q_OBJECT

public:
    explicit NumberButton(int value, QWidget *parent = nullptr);

signals:
    void clicked_number(int value);

private slots:
    void click_processing();

private:
    int value = INIT_VALUE;
};
