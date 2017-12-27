#ifndef CAESIUM_H
#define CAESIUM_H

#include <QMainWindow>

namespace Ui {
class Caesium;
}

class Caesium : public QMainWindow
{
    Q_OBJECT

public:
    explicit Caesium(QWidget *parent = 0);
    ~Caesium();

private:
    Ui::Caesium *ui;

    void initUI();
};

#endif // CAESIUM_H
