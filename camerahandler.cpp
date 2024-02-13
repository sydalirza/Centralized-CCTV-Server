#include "camerahandler.h"
#include <QDebug>
#include <QImageReader>
#include <QThread>


CameraHandler:: CameraHandler(QObject *parent) : QObject(parent), timer(new QTimer(this))
{
    connect(timer, &QTimer::timeout, this, &CameraHandler::updateFrames);
    timer->start(33); //FPS
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
    for (const auto& camera : cameras)
    {
        if (camera.cameraname == cameraname)
        {
            qDebug() << "Camera" << cameraname << " already open!";
            return;
        }
    }

    // Disconnect any existing connections for the current camera
    QObject::disconnect(&openTimer, &QTimer::timeout, nullptr, nullptr);

    openTimer.setSingleShot(true);

    // Connect the timer timeout to a lambda function for camera opening attempt
    QObject::connect(&openTimer, &QTimer::timeout, [this, cameraUrl, cameraname]() {
        // Attempt to open the camera
        VideoCapture videoCapture(cameraUrl);
        if (videoCapture.isOpened()) {
            CameraInfo newcamera;
            newcamera.videoCapture = videoCapture;
            newcamera.cameraname = cameraname;
            newcamera.cameraUrl = cameraUrl;

            qDebug() << "Opening " << cameraname;

            cameras.push_back(newcamera);
        }
    });

    // Start the timer with a timeout (adjust the timeout value as needed)
    openTimer.start(500);  // 5000 milliseconds (5 seconds)

    // Wait for the timer to finish
    while (openTimer.isActive()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }

    // Check if the camera was opened
    for (const auto& camera : cameras)
    {
        if (camera.cameraname == cameraname)
        {
            return; // Camera was opened
        }
    }

    // Timeout reached, assume camera opening failed
    qDebug() << "Camera opening attempt timed out." << cameraname << "1 ";


}


void CameraHandler::OpenCamera_single(const string &cameraUrl, const QString &cameraname)
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
    newcamera.cameraUrl = cameraUrl;

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
    auto it = find_if(cameras.begin(), cameras.end(), [cameraname](const CameraInfo &camera) {
        return camera.cameraname == cameraname;
    });

    static QImage errorFrame;
    QImageReader image("E:/FYP/image.png");
    errorFrame = image.read();

    if (it != cameras.end())
    {
        // Check if the latestFrame is the error image
        if (it->latestFrame.isNull() || it->latestFrame == errorFrame)
        {
            return errorFrame;
        }
        else
        {
            return it->latestFrame;
        }
    }

    // Camera not found, return the error image
    return errorFrame;
}

void CameraHandler::updateFrames()
{
    for (auto &camera : cameras)
    {
        Mat frame;
        camera.videoCapture.read(frame);
        QImageReader image("E:/FYP/image.png");
        QImage errorImage = image.read();

        if (frame.empty())
        {
            if (!camera.isError)
            {
                qDebug() << "Error reading frame from " << camera.cameraname;
                camera.isError = true;

                if (attemptReconnect(camera))
                {
                    qDebug() << "Reconnected to " << camera.cameraname;
                    camera.isError = false;
                }

                else
                {
                    qDebug() << "Unable to reconnect to " << camera.cameraname;
                }
            }
            else
            {
                camera.latestFrame = errorImage;
            }
        }
        else
        {
            camera.isError = false;
            camera.latestFrame = matToImage(frame);
        }

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

bool CameraHandler::getCameraError(const QString &cameraName) const
{
    auto it = find_if(cameras.begin(), cameras.end(), [cameraName](const CameraInfo &camera) {
        return camera.cameraname == cameraName;
    });

    if (it != cameras.end())
    {
        return it->isError;
    }
    else
    {
        return true; // or some default value for invalid cameraName
    }
}

string CameraHandler::getCameraUrl(const QString &cameraName) const
{
    auto it = find_if(cameras.begin(), cameras.end(), [cameraName](const CameraInfo &camera) {
        return camera.cameraname == cameraName;
    });

    if (it != cameras.end())
    {
        return it->cameraUrl;
    }
    else
    {
        return ""; // or some default value for invalid cameraName
    }
}

bool CameraHandler::attemptReconnect(CameraInfo &camera)
{
    qDebug() << "Attempting to reconnect with " << camera.cameraname;

    // Use a separate thread for reconnection attempt
    QThread* reconnectThread = new QThread;
    QTimer* reconnectTimer = new QTimer;

    // Connect the timer timeout to a lambda function for reconnection attempt
    QObject::connect(reconnectTimer, &QTimer::timeout, [reconnectTimer, reconnectThread, &camera]() {
        // Attempt to reopen the camera
        camera.videoCapture.open(camera.cameraUrl);
        // If the reconnection was successful, stop the timer and quit the thread
        if (camera.videoCapture.isOpened()) {
            reconnectTimer->stop();
            reconnectThread->quit();
        }
    });

    // Start the timer with a timeout (adjust the timeout value as needed)
    reconnectTimer->start(5000);  // 5000 milliseconds (5 seconds)

    // Move the timer to the separate thread
    reconnectTimer->moveToThread(reconnectThread);

    // Connect the thread's finished signal to the thread's deleteLater slot
    QObject::connect(reconnectThread, &QThread::finished, reconnectThread, &QThread::deleteLater);

    // Connect the thread's finished signal to stop the timer and delete it
    QObject::connect(reconnectThread, &QThread::finished, reconnectTimer, &QTimer::stop);
    QObject::connect(reconnectThread, &QThread::finished, reconnectTimer, &QTimer::deleteLater);

    // Start the thread
    reconnectThread->start();

    // Wait for the thread to finish with a timeout
    if (!reconnectThread->wait(10000)) {
        // Timeout reached, assume reconnection failed
        qDebug() << "Reconnection attempt timed out.";
        return false;
    }

    // Return true if the reconnection was successful
    return camera.videoCapture.isOpened();
}

void CameraHandler::printConnectedCameras() const
{
    qDebug() << "Connected Cameras:";

    for (const CameraInfo& camera : cameras)
    {
        qDebug() << camera.cameraname;
    }
}


