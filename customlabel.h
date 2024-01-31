#ifndef CUSTOMLABEL_H
#define CUSTOMLABEL_H

#include <QLabel>

class CustomLabel : public QLabel
{
    Q_OBJECT

public:
    explicit CustomLabel(QWidget *parent = nullptr);
    ~CustomLabel();

signals:
    void clicked();
    void doubleClicked();


protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
};

#endif // CUSTOMLABEL_H
