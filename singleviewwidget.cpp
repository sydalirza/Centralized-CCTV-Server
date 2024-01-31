// singleviewwidget.cpp

#include "singleviewwidget.h"
#include <QVBoxLayout>

SingleViewWidget::SingleViewWidget(QWidget *parent) : QWidget(parent)
{
    // Create the layout and QLabel for displaying the image
    QVBoxLayout *layout = new QVBoxLayout(this);
    imageLabel = new QLabel(this);
    layout->addWidget(imageLabel);
}

void SingleViewWidget::loadImage(const QPixmap &pixmap)
{
    // Display the image in the QLabel
    imageLabel->setPixmap(pixmap);
    imageLabel->setScaledContents(true);
}
