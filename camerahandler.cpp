// CameraHandler.cpp
#include "CameraHandler.h"
#include <opencv2/opencv.hpp>
#include <QDebug>

using namespace cv;

CameraHandler::CameraHandler(QObject *parent) : QObject(parent), timer(new QTimer(this))
{
    connect(timer, &QTimer::timeout, this, &CameraHandler::updateFrame);
    timer->start(33);  // Update every 33 milliseconds (approximately 30 FPS)
}

CameraHandler::~CameraHandler()
{
    closeAllCameras();
}

void CameraHandler::openCamera(const std::string& cameraUrl, const QString& cameraName)
{
    // Check if the camera is already open
    for (const auto &camera : cameras)
    {
        if (camera.cameraName == cameraName)
        {
            qDebug() << "Camera " << cameraName << " is already open.";
            return;
        }
    }

    // Open the camera using OpenCV
    VideoCapture videoCapture(cameraUrl);
    if (!videoCapture.isOpened())
    {
        qDebug() << "Error opening camera " << cameraName;
        return;
    }

    // Create a new CameraInfo structure
    CameraInfo newCamera;
    newCamera.videoCapture = videoCapture;
    newCamera.cameraName = cameraName;

    // Add the new camera to the list
    cameras.push_back(newCamera);
}

void CameraHandler::updateFrame()
{
    for (auto &camera : cameras)
    {
        // Read frame from the camera
        Mat frame;
        camera.videoCapture.read(frame);

        if (frame.empty())
        {
            qDebug() << "Error reading frame from camera " << camera.cameraName;
            emit frameUpdated(frame, camera.cameraName);
            return;
        }

        // Emit signal with the updated frame and camera name
        emit frameUpdated(frame, camera.cameraName);
    }
}

void CameraHandler::closeAllCameras()
{
    for (auto& camera : cameras)
    {
        camera.videoCapture.release();
    }

    cameras.clear();
}
