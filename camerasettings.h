#ifndef CAMERASETTINGS_H
#define CAMERASETTINGS_H

#include <QWidget>

namespace Ui {
class CameraSettings;
}

class CameraSettings : public QWidget
{
    Q_OBJECT

public:
    explicit CameraSettings(QWidget *parent = nullptr);
    ~CameraSettings();

private:
    Ui::CameraSettings *ui;
};

#endif // CAMERASETTINGS_H
