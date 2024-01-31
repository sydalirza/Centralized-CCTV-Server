#include "customlabel.h"

CustomLabel::CustomLabel(QWidget *parent) : QLabel(parent) {}

CustomLabel::~CustomLabel() {}

void CustomLabel::mousePressEvent(QMouseEvent *event)
{
    emit clicked();
    QLabel::mousePressEvent(event);
}

void CustomLabel::mouseDoubleClickEvent(QMouseEvent *event)
{
    emit doubleClicked();
    QLabel::mouseDoubleClickEvent(event);
}
