#include "camerahandler.h"
#include <QDebug>
#include <QImageReader>
#include <QThread>
#include <QtConcurrent/QtConcurrent>



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
        VideoCapture videoCapture(cameraUrl, CAP_FFMPEG);
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
    openTimer.start(500);  // 500 milliseconds (adjust as needed)

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
    emit cameraOpeningFailed(cameraname);
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
    auto it = std::remove_if(cameras.begin(), cameras.end(), [cameraname](const CameraInfo &camera) {
        return camera.cameraname == cameraname;
    });

    if (it != cameras.end())
    {
        qDebug() << "Removing " << it->cameraname;
        // Release resources before removing the camera
        it->videoCapture.release();
        cameras.erase(it, cameras.end());
    }
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


void CameraHandler::reconnectCamera(CameraInfo& camera)
{
    // Attempt to reopen the camera
    camera.videoCapture.open(camera.cameraUrl, CAP_FFMPEG);

    // If the reconnection was successful, set the flag to false
    if (camera.videoCapture.isOpened()) {
        qDebug() << "Reconnected " << camera.cameraname;
        camera.isError = false;
        camera.isReconnecting = false;
    }

    // If all reconnection attempts failed, remove the camera
    if (camera.isError) {
        qDebug() << "Removing " << camera.cameraname << " due to disconnection";
        emit removeCamera(camera.cameraname);
    }
}

void CameraHandler::updateFrames()
{
    QVector<QFuture<void>> futures;
    QVector<QFutureWatcher<void>*> watchers;

    for (auto &camera : cameras)
    {
        if (camera.isReconnecting) {
            // Skip processing frames if the camera is reconnecting
            continue;
        }

        if (camera.isError) {
            qDebug() << "Attempting to reconnect for " << camera.cameraname << " is running on thread" << QThread::currentThreadId();
            camera.isReconnecting = true;

            // Use a separate thread for reconnection attempt
            QFuture<void> future = QtConcurrent::run([this, &camera]() {
                reconnectCamera(camera);
            });

            futures.append(future);
        }
        else {
            // Process frames for each camera concurrently
            QFuture<void> future = QtConcurrent::run([this, &camera]() {
                processFrame(camera);
            });

            futures.append(future);
        }
    }

    // Wait for all futures to finish
    for (const auto& future : futures)
    {
        QFutureWatcher<void>* watcher = new QFutureWatcher<void>;
        QObject::connect(watcher, &QFutureWatcher<void>::finished, watcher, &QFutureWatcher<void>::deleteLater);
        watcher->setFuture(future);

        watchers.append(watcher);
    }

    // Note: Do not use waitForFinished() here
    // Let the event loop handle the completion of futures asynchronously
}

void CameraHandler::processFrame(CameraInfo& camera)
{
    // qDebug() << "Frame processing for " << camera.cameraname << " is running on thread" << QThread::currentThreadId();
    Mat frame;
    camera.videoCapture.read(frame);

    if (frame.empty()) {
        qDebug() << "Error reading frame from " << camera.cameraname << " is running on thread" << QThread::currentThreadId();
        camera.isError = true;
    }
    else {
        // qDebug() << "Reading frame from " << camera.cameraname << " is running on thread" << QThread::currentThreadId();
        camera.latestFrame = matToImage(frame);
    }

    emit frameUpdated(camera.latestFrame, camera.cameraname);
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

// bool CameraHandler::attemptReconnect(CameraInfo &camera)
// {
//     qDebug() << "Attempting to reconnect with " << camera.cameraname;

//     // Use a separate thread for reconnection attempt
//     QThread* reconnectThread = new QThread;
//     QTimer* reconnectTimer = new QTimer;

//     // Connect the timer timeout to a lambda function for reconnection attempt
//     QObject::connect(reconnectTimer, &QTimer::timeout, [reconnectTimer, &camera]() {
//         // Attempt to reopen the camera
//         camera.videoCapture.open(camera.cameraUrl);
//         // If the reconnection was successful, stop the timer and emit reconnected signal
//         if (camera.videoCapture.isOpened()) {
//             reconnectTimer->stop();
//             emit cameraReconnected(camera.cameraname);
//         }
//     });

//     // Connect the thread's finished signal to stop the timer and delete it
//     QObject::connect(reconnectThread, &QThread::finished, reconnectTimer, &QTimer::stop);
//     QObject::connect(reconnectThread, &QThread::finished, reconnectTimer, &QTimer::deleteLater);

//     // Connect the timer's timeout to delete the timer
//     QObject::connect(reconnectTimer, &QTimer::timeout, reconnectTimer, &QTimer::deleteLater);

//     // Start the thread
//     reconnectThread->start();

//     // Start the timer with a timeout (adjust the timeout value as needed)
//     reconnectTimer->start(5000);  // 5000 milliseconds (5 seconds)

//     // Return true to indicate that the reconnection process has started
//     return true;
// }


void CameraHandler::printConnectedCameras() const
{
    qDebug() << "Connected Cameras:";

    for (const CameraInfo& camera : cameras)
    {
        qDebug() << camera.cameraname;
    }
}


