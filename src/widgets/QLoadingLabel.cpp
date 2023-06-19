#include "QLoadingLabel.h"

#include <QMovie>

QLoadingLabel::QLoadingLabel(QWidget* parent)
    : QLabel(parent)
{
    this->animationMovie = new QMovie(":/icons/ui/loader.webp");
}

QLoadingLabel::~QLoadingLabel()
{
    delete this->animationMovie;
}

void QLoadingLabel::setLoading(bool l)
{
    this->loading = l;

    if (l) {
        this->clear();
        this->animationMovie->setScaledSize(QSize(this->fontMetrics().height(), this->fontMetrics().height()));
        this->setMovie(this->animationMovie);
        this->animationMovie->start();
    } else {
        this->animationMovie->stop();
        this->setMovie(nullptr);
    }
}