#ifndef QCOLLAPSETOOLBUTTON_H
#define QCOLLAPSETOOLBUTTON_H

#include <QFrame>
#include <QToolButton>

class QCollapseToolButton : public QToolButton {
    Q_OBJECT

public:
    explicit QCollapseToolButton(QWidget* parent = nullptr);
    ~QCollapseToolButton() override;

    void setContent(QFrame* frame);
    bool contentVisible() const;
    void setContentVisible(bool visible);

protected:
    QFrame* content;
    bool isContentVisible;

public slots:
    void toggleContentVisible();

signals:
    void contentVisibilityToggled(bool visible);
};

#endif // QCOLLAPSETOOLBUTTON_H
