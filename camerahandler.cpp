#include "camerahandler.h"
#include <QDebug>
#include <QImageReader>


CameraHandler:: CameraHandler(QObject *parent) : QObject(parent), timer(new QTimer(this))
{
    connect(timer, &QTimer::timeout, this, &CameraHandler::updateFrames);
    timer->start(33);

    // OpenCamera("rtsp://192.168.1.3:8080/h264.sdp", "Camera 1");

    OpenCamera("C:/Users/Yousuf Traders/Downloads/1.mp4", "Camera 2");

    OpenCamera("C:/Users/Yousuf Traders/Downloads/2.mp4", "Camera 3");

    OpenCamera("C:/Users/Yousuf Traders/Downloads/2.mp4", "Camera 4");

    OpenCamera("C:/Users/Yousuf Traders/Downloads/2.mp4", "Camera 5");
}

CameraHandler:: ~CameraHandler(){
    closeAllCameras();
}

void CameraHandler::closeAllCameras()
{
    for (auto& camera : cameras)
    {
        camera.videoCapture.release();
    }

    cameras.clear();
}

void CameraHandler::OpenCamera(const string &cameraUrl, const QString &cameraname)
{
    for (auto& camera: cameras)
    {
        if(camera.cameraname == cameraname)
        {
        qDebug() << "Camera" << cameraname << " already open!";
        return;
    }
    }

    VideoCapture videoCapture(cameraUrl);
    if(!videoCapture.isOpened())
    {
        qDebug() << "Error opening Camera" << cameraname;
        return;
    }

    CameraInfo newcamera;
    newcamera.videoCapture = videoCapture;
    newcamera.cameraname = cameraname;

    cameras.push_back(newcamera);
}

void CameraHandler::CloseCamera(const QString &cameraname)
{
    cameras.erase(remove_if(cameras.begin(), cameras.end(), [cameraname](const CameraInfo &camera)
    {
        return camera.cameraname == cameraname;
                            }),cameras.end());
}

const QImage &CameraHandler::getLatestFrame(const QString &cameraname) const
{
    auto it = find_if(cameras.begin(), cameras.end(), [cameraname](const CameraInfo &camera){
        return camera.cameraname == cameraname;
    });
    if(it != cameras.end())
    {
        return it-> latestFrame;
    }

    static QImage errorFrame;
    QImageReader image("error.png");
    errorFrame = image.read();
    return errorFrame;
}

void CameraHandler::updateFrames()
{
    for(auto &camera: cameras)
    {
        Mat frame;
        camera.videoCapture.read(frame);

        if(frame.empty())
        {
            qDebug() << "Error reading frame from " << camera.cameraname;
            continue;
        }
        camera.latestFrame = matToImage(frame);

        emit frameUpdated(camera.latestFrame, camera.cameraname);
    }
}

QImage CameraHandler::matToImage(const Mat &mat) const
{
    if (mat.channels() == 3)
    {
        QImage image(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        return image.rgbSwapped();
    }

    else if(mat.channels() == 1)
    {
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8);
    }

    qDebug() << "Unsupported Image Format";
    return QImage();
}

int CameraHandler::getNumberOfConnectedCameras() const
{
    return cameras.size();
}

QString CameraHandler::getCameraName(int index) const
{
    if (index >= 0 && index < cameras.size())
        return cameras.at(index).cameraname;
    else
        return QString(); // or some default value for invalid index
}
