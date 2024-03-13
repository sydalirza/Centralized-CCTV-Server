#ifndef CAMERASETTINGS_H
#define CAMERASETTINGS_H

#include <QWidget>
using namespace std;

namespace Ui {
class CameraSettings;
}

class CameraSettings : public QWidget
{
    Q_OBJECT

public:
    explicit CameraSettings(QWidget *parent = nullptr);
    ~CameraSettings();

signals:
    void add_camera(const QString &cameraurl, const QString &cameraname);

private slots:
    void on_addbutton_clicked();

    void on_rtsp_radiobutton_clicked();

    void on_mp4_radiobutton_clicked();

    void on_clearbutton_clicked();

private:
    Ui::CameraSettings *ui;
    bool rtsp = false;
    bool mp4 = false;

};

#endif // CAMERASETTINGS_H
