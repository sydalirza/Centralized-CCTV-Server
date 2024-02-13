// singleviewwidget.h
#ifndef SINGLEVIEWWIDGET_H
#define SINGLEVIEWWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QThread>
#include "camerahandler.h"

class SingleViewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SingleViewWidget(QWidget *parent = nullptr, const QString& cameraName = "", const std::string& cameraUrl = "");
    ~SingleViewWidget();

public slots:
    void updateImage(const QImage &frame);
    void openCamera(const std::string &cameraUrl, const QString &cameraName);
    void closeCamera();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QLabel *imageLabel;
    QString displayedCamera;
    QThread cameraThread;
    CameraHandler cameraHandler;

signals:
    void openCameraSignal(const std::string &cameraUrl, const QString &cameraName);
    void closeCameraSignal();
};

#endif // SINGLEVIEWWIDGET_H
