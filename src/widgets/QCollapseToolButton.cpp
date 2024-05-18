#include "QCollapseToolButton.h"

QCollapseToolButton::QCollapseToolButton(QWidget* parent)
    : QToolButton(parent)
{
#ifdef Q_OS_MAC
    this->setStyleSheet("border: none;");
#endif
    this->content = new QFrame();
    this->isContentVisible = true;
    this->setArrowType(Qt::ArrowType::DownArrow);
    this->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    this->setAutoRaise(true);
    this->setIconSize(QSize(8, 8));

    connect(this, &QToolButton::clicked, this, &QCollapseToolButton::toggleContentVisible);
}

QCollapseToolButton::~QCollapseToolButton()
    = default;

bool QCollapseToolButton::contentVisible() const
{
    return this->isContentVisible;
}

void QCollapseToolButton::setContentVisible(bool visible)
{
    this->isContentVisible = visible;
    this->setArrowType(this->isContentVisible ? Qt::ArrowType::DownArrow : Qt::ArrowType::RightArrow);
    this->content->setVisible(visible);
}

void QCollapseToolButton::setContent(QFrame* frame)
{
    this->content = frame;
    this->content->setVisible(this->isContentVisible);
}

void QCollapseToolButton::toggleContentVisible()
{
    this->isContentVisible = !this->isContentVisible;
    this->content->setVisible(this->isContentVisible);
    this->setArrowType(this->isContentVisible ? Qt::ArrowType::DownArrow : Qt::ArrowType::RightArrow);

    emit contentVisibilityToggled(this->isContentVisible);
}
