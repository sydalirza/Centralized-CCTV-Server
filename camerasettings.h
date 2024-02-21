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

private slots:
    void on_addbutton_clicked();

    void on_rtsp_radiobutton_clicked();

    void on_mp4_radiobutton_clicked();

private:
    Ui::CameraSettings *ui;
};

#endif // CAMERASETTINGS_H
