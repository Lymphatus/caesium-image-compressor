#ifndef UPDATER_H
#define UPDATER_H

#include <QMainWindow>

namespace Ui {
class Updater;
}

class Updater : public QMainWindow
{
    Q_OBJECT

public:
    explicit Updater(QWidget *parent = 0);
    ~Updater();

private:
    Ui::Updater *ui;
};

#endif // UPDATER_H
