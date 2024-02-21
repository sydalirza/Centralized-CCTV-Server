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
    Mat facedetection(Mat frame);


protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QLabel *imageLabel;
    QString displayedCamera;
    QThread cameraThread;
    CameraHandler cameraHandler;
    CascadeClassifier face_cascade;
    string faceClassifier = "C:/Users/Yousuf Traders/Downloads/opencv/sources/data/haarcascades/haarcascade_frontalface_alt2.xml";
    Mat ImagetoMat(const QImage &image);
    QImage matToImage(const Mat &mat) const;
    bool detectfaces = false;



signals:
    void openCameraSignal(const std::string &cameraUrl, const QString &cameraName);
    void closeCameraSignal();
};

#endif // SINGLEVIEWWIDGET_H
