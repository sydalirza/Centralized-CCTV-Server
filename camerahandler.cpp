#include "camerahandler.h"
#include "recordingworker.h"

#include <QDebug>
#include <QImageReader>
#include <QThread>
#include <QtConcurrent/QtConcurrent>
#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>

using namespace cv;
using namespace cv::face;


CameraHandler:: CameraHandler(QObject *parent) : QObject(parent), timer(new QTimer(this))
{
    connect(timer, &QTimer::timeout, this, &CameraHandler::updateFrames);
    timer->start(1); //FPS

    string faceClassifier = "haarcascade_frontalface_alt2.xml";

    if (!faceCascade.load(faceClassifier)) {
        qDebug() << "Could not load the classifier";
        QCoreApplication::exit(-1);
    }

    qDebug() << "Classifier Loaded!";

    recognizer->read("trained_model.yml");


    QDir videoDir(videoFolder);
    if (!videoDir.exists()) {
        videoDir.mkpath(".");
    }
}

CameraHandler:: ~CameraHandler(){
    closeAllCameras();
}

void CameraHandler::closeAllCameras()
{
    for (auto& camera : cameras)
    {
        camera.videoCapture.release();
        camera.videoWriter.release();
    }

    cameras.clear();
}

void CameraHandler::initializeVideoWriter(const QString &cameraname)
{
    // Find the camera with the specified name
    auto it = std::find_if(cameras.begin(), cameras.end(), [cameraname](const CameraInfo &camera) {
        return camera.cameraname == cameraname;
    });

    if (it != cameras.end()) {
        // Set up the directory path for the camera
        QString cameraDirPath = videoFolder + "/" + cameraname;

        // Create the directory for the camera if it doesn't exist
        QDir cameraDir(cameraDirPath);
        if (!cameraDir.exists()) {
            if (cameraDir.mkpath(cameraDirPath)) {
                qDebug() << "Directory created successfully: " << cameraDirPath;
            } else {
                qDebug() << "Error: Failed to create directory: " << cameraDirPath;
            }
        }

        // Set up VideoWriter with codec XVID and 20 FPS
        int codec = VideoWriter::fourcc('X','V','I','D');
        Size frameSize(it->videoCapture.get(CAP_PROP_FRAME_WIDTH), it->videoCapture.get(CAP_PROP_FRAME_HEIGHT));

        it->videoWriter.open(cameraDirPath.toStdString() + "/video_" + cameraname.toStdString() + ".avi", codec, 30, frameSize);

        if (it->videoWriter.isOpened()) {
            qDebug() << "VideoWriter opened successfully.";
        } else {
            qDebug() << "Error: VideoWriter failed to open!";
        }
    } else {
        qDebug() << "Camera not found: " << cameraname;
    }
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

            initializeVideoWriter(cameraname);
        }
    });

    // Start the timer with a timeout (adjust the timeout value as needed)
    openTimer.start(300);  // 5000 milliseconds (adjust as needed)

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
    qDebug() << "Camera opening attempt timed out." << cameraname;
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

        it->videoWriter.release();

        it->frameBuffer.clear();

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
            qDebug() << "Attempting to reconnect for " << camera.cameraname;
            camera.isReconnecting = true;

            // Use a separate thread for reconnection attempt
            QFuture<void> future = QtConcurrent::run([this, &camera]() {
                reconnectCamera(camera);
            });

            futures.append(future);
        }
        else {
            // Process frames for each camera concurrently
            // QFuture<void> future = QtConcurrent::run([this, &camera]() {

            // });
            processFrame(camera);

            // futures.append(future);
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
}

Mat CameraHandler::facedetection(Mat frame, CameraInfo &camera) {

    // Resize the input frame to a smaller size for faster processing
    Mat resizedFrame;
    constexpr double scale = 0.07; // Adjust the scale factor as needed
    resize(frame, resizedFrame, Size(), scale, scale);

    // Set confidence threshold
    const double confidenceThreshold = 85.0; // Adjust this value as needed

    // Convert the resized frame to grayscale
    Mat frame_gray;
    cvtColor(resizedFrame, frame_gray, COLOR_BGR2GRAY);

    // Detect faces in the resized grayscale frame
    vector<Rect> faces;
    double scaleFactor = 1.5; // Experiment with different values (e.g., 1.1, 1.2, etc.)
    int minNeighbors = 3; // Experiment with different values (e.g., 3, 5, 7, etc.)
    int flags = 0;
    Size minSize(30, 30); // Experiment with different minimum sizes
    Size maxSize(300, 300); // Experiment with different maximum sizes
    faceCascade.detectMultiScale(frame_gray, faces, scaleFactor, minNeighbors, flags, minSize, maxSize);

    // Check the number of detected faces
    if (faces.empty())
    {
        if (camera.isRecording && camera.persondetected && camera.frameBuffer.length() >= camera.startFrameIndex + 100)
        {
            qDebug() << "Person has left the frame";
            camera.endFrameIndex = camera.frameBuffer.length()-10;
            qDebug() << "Start = " << camera.startFrameIndex << " End = " << camera.endFrameIndex << "Current = " << camera.frameBuffer.length();

            RecordingWorker* worker = new RecordingWorker;

            // Move the worker object to a separate thread
            // Create a new thread
            // Create a new thread
            QThread* recordingThread = new QThread;

            // Move the RecordingWorker instance to the new thread
            worker -> moveToThread(recordingThread);

            // Call recordvideo from the new thread using lambda function
            QObject::connect(recordingThread, &QThread::started, [=]() {
                worker -> recordvideo(camera.startFrameIndex, camera.endFrameIndex, camera.cameraname, camera.CameraRecording);
            });

            // Connect thread's finished signal to deleteLater() slot to clean up when the thread finishes
            QObject::connect(recordingThread, &QThread::finished, recordingThread, &QThread::deleteLater);

            // Start the thread
            recordingThread->start();

            // No faces detected, reset persondetected
            camera.persondetected = false;
            camera.isRecording = false;
            camera.cooldowntime = camera.endFrameIndex;
        }
    }
    else {
        // At least one face detected
        for (const Rect& face : faces)
        {
            // Extract face region
            Mat faceROI = frame_gray(face);

            // Perform face recognition
            int label = -1;
            double confidence = 0.0;
            recognizer->predict(faceROI, label, confidence);

            // Display recognized face label if confidence is above threshold
            if (label != -1 && confidence < confidenceThreshold) {
                // Draw green rectangle and put recognized label
                rectangle(resizedFrame, face, Scalar(0, 255, 0), 1);
            }
            else {
                // Draw red rectangle for unknown face or low-confidence prediction
                rectangle(resizedFrame, face, Scalar(0, 0, 255), 1);
            }
        }

        // Print a detection message based on the number of detected faces
        if (faces.size() >= 1 && !camera.persondetected && !camera.isRecording)
        {
            if (camera.frameBuffer.length() - camera.cooldowntime <= 200 && camera.cooldowntime != 0)
            {
                qDebug() << "Cooling down";
            }
            else
            {
                QDateTime currentDateTime = QDateTime::currentDateTime();
                QString formattedDateTime = currentDateTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
                qDebug() << "Person detected in the " << camera.cameraname << " camera at " << formattedDateTime;
                camera.persondetected = true;
                if (camera.frameBuffer.length() >= 100 && !camera.isRecording)
                {
                    camera.startFrameIndex = camera.frameBuffer.length() - 100;
                    camera.isRecording = true;
                }
                else if (camera.frameBuffer.length() < 100 && !camera.isRecording)
                {
                    camera.startFrameIndex = 0;
                    camera.isRecording = true;
                }
            }

        }
    }

    return resizedFrame;
}


void CameraHandler::processFrame(CameraInfo& camera)
{
    QDateTime currentDateTime = QDateTime::currentDateTime();
    Mat frame;
    camera.videoCapture.read(frame);

    if (frame.empty() && !camera.isError) {
        qDebug() << "Error reading frame from " << camera.cameraname;
        camera.isError = true;

        Mat blackFrame(1, 1, CV_8UC3, cv::Scalar(0, 0, 0));
        camera.frameBuffer.append(qMakePair(blackFrame, currentDateTime.time()));

        // Append the frame buffer to CameraRecording with the current date
        camera.CameraRecording.append(qMakePair(currentDateTime.date(), qMakePair(blackFrame, currentDateTime.time())));
    }
    else {
        Mat AIframe = facedetection(frame, camera);
        camera.latestFrame = matToImage(AIframe);

        camera.frameBuffer.append(qMakePair(AIframe, currentDateTime.time()));

        // Append the frame buffer to CameraRecording with the current date
        camera.CameraRecording.append(qMakePair(currentDateTime.date(), qMakePair(AIframe, currentDateTime.time())));
    }

    serialize(camera);
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

void CameraHandler::printConnectedCameras() const
{
    qDebug() << "Connected Cameras:";

    for (const CameraInfo& camera : cameras)
    {
        qDebug() << camera.cameraname;
    }
}

QVector<QPair<QDate, QPair<Mat, QTime>>> CameraHandler::getFrameBuffer(const QString& cameraname) const {
    auto it = std::find_if(cameras.begin(), cameras.end(), [cameraname](const CameraInfo& camera) {
        return camera.cameraname == cameraname;
    });

    if (it != cameras.end()) {
        return it->CameraRecording;
    }

    return QVector<QPair<QDate, QPair<Mat, QTime>>>(); // Return empty buffer if not found
}

void CameraHandler::clearFrameBuffer(const QString& cameraname) {
    auto it = std::find_if(cameras.begin(), cameras.end(), [cameraname](const CameraInfo& camera) {
        return camera.cameraname == cameraname;
    });

    if (it != cameras.end()) {
        it->frameBuffer.clear();
    }
}

void serializeMat(QDataStream &stream, const Mat &mat)
{
    // Convert the Mat object to a QByteArray for serialization
    QByteArray matData;
    QDataStream dataStream(&matData, QIODevice::WriteOnly);
    dataStream << mat.cols << mat.rows << mat.type() << QByteArray((char*)mat.data, mat.total() * mat.elemSize());

    // Serialize the QByteArray containing Mat data
    stream << matData;
}

void CameraHandler::serialize(CameraInfo &camera)
{
    QFile file(camera.cameraname);
    if (file.open(QIODevice::WriteOnly)) {
        QDataStream out(&file);

        // Serialize the number of frames in the frame buffer
        out << camera.frameBuffer.size();

        // Serialize each frame (Mat object) and its corresponding QTime
        for (const auto& framePair : camera.frameBuffer) {
            // Serialize the QTime
            out << framePair.second;

            // Serialize the Mat object
            serializeMat(out, framePair.first);
        }

        file.close();
    }
    else
    {
        qDebug() << "Error opening file for writing:" << file.errorString();
    }
}


