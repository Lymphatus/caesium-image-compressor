#ifndef QLOADINGLABEL_H
#define QLOADINGLABEL_H

#include <QLabel>

class QLoadingLabel : public QLabel {
    Q_OBJECT

public:
    explicit QLoadingLabel(QWidget* parent = 0);
    ~QLoadingLabel() override;

    void setLoading(bool l = true);

private:
    bool loading = false;
    QMovie* animationMovie;
};

#endif // QLOADINGLABEL_H
