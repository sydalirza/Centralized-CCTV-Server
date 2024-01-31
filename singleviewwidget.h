// singleviewwidget.h

#ifndef SINGLEVIEWWIDGET_H
#define SINGLEVIEWWIDGET_H

#include <QWidget>
#include <QLabel>

class SingleViewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SingleViewWidget(QWidget *parent = nullptr);
    void loadImage(const QPixmap &pixmap);

private:
    QLabel *imageLabel;
};

#endif // SINGLEVIEWWIDGET_H
