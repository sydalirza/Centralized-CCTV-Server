#include "camerahandler.h"
#include "recordingworker.h"

#include <QDebug>
#include <QImageReader>
#include <QThread>
#include <QMessageBox>
#include <QProgressDialog>
#include <QtConcurrent/QtConcurrent>
#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include <dlib/opencv.h>
#include <dlib/image_processing.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/image_io.h>
#include <dlib/clustering.h>
#include <dlib/string.h>
#include <dlib/dnn.h>

CameraHandler:: CameraHandler(QObject *parent) : QObject(parent), timer(new QTimer(this))
{
    connect(timer, &QTimer::timeout, this, &CameraHandler::updateFrames);
    timer->start(1); //FPS

    // Connect cleanup timer to cleanupOldFrames method
    connect(&cleanupTimer, &QTimer::timeout, this, &CameraHandler::cleanupOldFrames);
    cleanupTimer.start(24 * 60 * 60 * 1000); // Run once every day

    std::string faceClassifier = "haarcascade_frontalface_alt2.xml";

    if (!faceCascade.load(faceClassifier)) {
        qDebug() << "Could not load the classifier";
        QCoreApplication::exit(-1);
    }

    qDebug() << "Classifier Loaded!";

    recognizer->read("trained_model.yml");

    db = QSqlDatabase::addDatabase("QSQLITE", "cameras_connection"); // Specify a unique connection name
    db.setDatabaseName("cameras.db");

    // Open the database connection
    if (!db.open()) {
        qDebug() << "Error: Failed to open the database.";
    } else {
        // Create the 'camera_logs' table if it doesn't exist
        QSqlQuery query(db); // Pass the database connection to QSqlQuery constructor
        if (!query.exec("CREATE TABLE IF NOT EXISTS camera_logs ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                        "camera_name TEXT,"
                        "file_name TEXT,"
                        "start_time TEXT,"
                        "end_time TEXT"
                        ")")) {
            qDebug() << "Error creating table:" << query.lastError().text();
        }
    }
}

CameraHandler:: ~CameraHandler(){
    qDebug() << "Closing Camera Handler";
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

void CameraHandler::cleanupOldFrames()
{
    QDate currentDate = QDate::currentDate();
    for (auto &camera : cameras) {
        camera.CameraRecording.erase(std::remove_if(camera.CameraRecording.begin(), camera.CameraRecording.end(),
                                                    [currentDate](const QPair<QDate, QPair<cv::Mat, QTime>> &record) {
                                                        return record.first < currentDate.addDays(-7);
                                                    }),
                                     camera.CameraRecording.end());
    }
}

void CameraHandler::OpenCamera(const std::string &cameraUrl, const QString &cameraname)
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
        cv::VideoCapture videoCapture(cameraUrl, cv::CAP_FFMPEG);

        if (videoCapture.isOpened()) {
            CameraInfo newcamera;

            newcamera.videoCapture = videoCapture;
            newcamera.cameraname = cameraname;
            newcamera.cameraUrl = cameraUrl;

            QString filePath = cameraname + ".dat";
            if (QFile::exists(filePath)) {
                // Ask the user if they want to reload the camera
                QMessageBox::StandardButton reply;
                reply = QMessageBox::question(nullptr, "Reload Camera", "A serialized file for this camera already exists. Do you want to reload the camera from the serialized file?",
                                              QMessageBox::Yes|QMessageBox::No);
                if (reply == QMessageBox::Yes) {
                    // User chose not to reload, so just return
                    deserialize(newcamera);
                }
            }

            // Ask the user if they want to arm the camera
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, "Arm Camera?", "Do you want to arm the camera?",
                                              QMessageBox::Yes|QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                newcamera.armed = true;
            }

            qDebug() << "Opening " << cameraname;

            cameras.push_back(newcamera);

            // initializeVideoWriter(cameraname);
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


void CameraHandler::OpenCamera_single(const std::string &cameraUrl, const QString &cameraname)
{
    for (auto& camera: cameras)
    {
        if(camera.cameraname == cameraname)
        {
            qDebug() << "Camera" << cameraname << " already open!";
            return;
        }
    }

    cv::VideoCapture videoCapture(cameraUrl);
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

        // Capture CameraRecording before erasing the camera
        QVector<QPair<QDate, QPair<cv::Mat, QTime>>> cameraRecording = it->CameraRecording;

        it->CameraRecording.clear();

        cameras.erase(it, cameras.end());

        qDebug() << "Saving frames";

        int startFrameindex = 0;
        qDebug() << startFrameindex;
        int endFrameindex = cameraRecording.size()-1;
        qDebug() << endFrameindex;

        RecordingWorker* worker = new RecordingWorker;

        // Move the worker object to a separate thread
        // Create a new thread
        QThread* recordingThread = new QThread;

        // Move the RecordingWorker instance to the new thread
        worker -> moveToThread(recordingThread);

        // Call recordvideo from the new thread using lambda function
        QObject::connect(recordingThread, &QThread::started, [=]() {
            worker -> recordvideo(startFrameindex, endFrameindex, cameraname, cameraRecording);
        });

        // Connect thread's finished signal to deleteLater() slot to clean up when the thread finishes
        QObject::connect(recordingThread, &QThread::finished, recordingThread, &QThread::deleteLater);

        // Start the thread
        recordingThread->start();

    }
}


const QImage &CameraHandler::getLatestFrame(const QString &cameraname) const
{
    auto it = std::find_if(cameras.begin(), cameras.end(), [cameraname](const CameraInfo &camera) {
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
    camera.videoCapture.open(camera.cameraUrl, cv::CAP_FFMPEG);
    qDebug() << "here";

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

cv::Mat CameraHandler::facedetection(cv::Mat frame, CameraInfo &camera) {
    // Resize the input frame to a smaller size for faster processing
    cv::Mat resizedFrame;
    cv::resize(frame, resizedFrame, cv::Size(), camera.scaleFactor, camera.scaleFactor);

    if(!camera.armed)
    {
        return resizedFrame;
    }

    // Set confidence threshold
    const double confidenceThreshold = 85.0; // Adjust this value as needed

    // Convert the resized frame to grayscale
    cv::Mat frame_gray;
    cvtColor(resizedFrame, frame_gray, cv::COLOR_BGR2GRAY);

    // Detect faces in the resized grayscale frame
    std::vector<cv::Rect> faces;
    double scaleFactor = 1.3; // Experiment with different values (e.g., 1.1, 1.2, etc.)
    int minNeighbors = 1; // Experiment with different values (e.g., 3, 5, 7, etc.)
    int flags = 0;
    faceCascade.detectMultiScale(frame_gray, faces, scaleFactor, minNeighbors, flags);

    static int unrecognizedCount = 0;
    // Check the number of detected faces
    if (faces.empty())
    {
        if (camera.isRecording && camera.persondetected && camera.CameraRecording.length() >= camera.startFrameIndex + 100)
        {
            qDebug() << "Person has left the frame";
            camera.endFrameIndex = camera.CameraRecording.length()-10;
            qDebug() << "Start = " << camera.startFrameIndex << " End = " << camera.endFrameIndex << "Current = " << camera.CameraRecording.length();

            RecordingWorker* worker = new RecordingWorker;

            // Move the worker object to a separate thread
            // Create a new thread
            QThread* recordingThread = new QThread;

            // Move the RecordingWorker instance to the new thread
            worker -> moveToThread(recordingThread);

            // Call recordvideo from the new thread using lambda function
            QObject::connect(recordingThread, &QThread::started, [=]() {
                worker -> recordvideo(camera.startFrameIndex, camera.endFrameIndex, camera.cameraname, camera.CameraRecording, db);
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
        for (const cv::Rect& face : faces)
        {
            // Extract face region
            cv::Mat faceROI = frame_gray(face);

            // Perform face recognition
            int label = -1;
            double confidence = 0.0;
            recognizer->predict(faceROI, label, confidence);

            // Display recognized face label if confidence is above threshold
            if (label != -1 && confidence < confidenceThreshold) {
                // Draw green rectangle and put recognized label
                rectangle(resizedFrame, face, cv::Scalar(0, 255, 0), 1);
            }
            else {
                // Draw red rectangle for unknown face or low-confidence prediction
                rectangle(resizedFrame, face, cv::Scalar(0, 0, 255), 1);
                // Save the detected face
                QString filename = QString("unrecognized_face_%1.jpg").arg(unrecognizedCount);
                QString filepath = QString("faces/") + filename; // Fix filepath construction
                qDebug() << filename;
                imwrite(filepath.toStdString(), faceROI);
                unrecognizedCount++;
            }
        }

        // Print a detection message based on the number of detected faces
        if (faces.size() >= 1 && !camera.persondetected && !camera.isRecording)
        {
            if (camera.CameraRecording.length() - camera.cooldowntime <= 200 && camera.cooldowntime != 0)
            {

            }
            else
            {
                QDateTime currentDateTime = QDateTime::currentDateTime();
                QString formattedDateTime = currentDateTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
                qDebug() << "Person detected in the " << camera.cameraname << " camera at " << formattedDateTime;
                camera.persondetected = true;
                if (camera.CameraRecording.length() >= 100 && !camera.isRecording)
                {
                    camera.startFrameIndex = camera.CameraRecording.length() - 100;
                    camera.isRecording = true;
                }
                else if (camera.CameraRecording.length() < 100 && !camera.isRecording)
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
    cv::Mat frame;
    cv::Mat newframe;
    camera.videoCapture.read(frame);

    if (frame.empty() && !camera.isError) {
        qDebug() << "Error reading frame from " << camera.cameraname;
        camera.isError = true;

        cv::Mat blackFrame(1, 1, CV_8UC3, cv::Scalar(0, 0, 0));

        // Append the frame buffer to CameraRecording with the current date
        camera.CameraRecording.append(qMakePair(currentDateTime.date(), qMakePair(blackFrame, currentDateTime.time())));
    }
    else {
        newframe = facedetection(frame, camera);

        camera.latestFrame = matToImage(newframe);

        // Append the frame buffer to CameraRecording with the current date
        camera.CameraRecording.append(qMakePair(currentDateTime.date(), qMakePair(newframe, currentDateTime.time())));
    }

    queueSerializationTask(camera);
    emit frameUpdated(camera.latestFrame, camera.cameraname);
}


QImage CameraHandler::matToImage(const cv::Mat &mat) const
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
    auto it = std::find_if(cameras.begin(), cameras.end(), [cameraName](const CameraInfo &camera) {
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

std::string CameraHandler::getCameraUrl(const QString &cameraName) const
{
    auto it = std::find_if(cameras.begin(), cameras.end(), [cameraName](const CameraInfo &camera) {
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

bool CameraHandler::getArmedStatus(const QString &cameraName) const
{
    auto it = std::find_if(cameras.begin(), cameras.end(), [cameraName](const CameraInfo &camera) {
        return camera.cameraname == cameraName;
    });

    if (it != cameras.end())
    {
        return it->armed;
    }
    else
    {
        return ""; // or some default value for invalid cameraName
    }
}

double CameraHandler::getScalefactor(const QString &cameraName)
{
    auto it = std::find_if(cameras.begin(), cameras.end(), [cameraName](const CameraInfo &camera) {
        return camera.cameraname == cameraName;
    });

    if (it != cameras.end())
    {
        return it->scaleFactor;
    }
    else
    {
        return 0.07; // or some default value for invalid cameraName
    }
}


void CameraHandler::changeCamerastatus(const QString &cameraName)
{
    auto it = std::find_if(cameras.begin(), cameras.end(), [cameraName](const CameraInfo &camera) {
        return camera.cameraname == cameraName;
    });

    if (it != cameras.end())
    {
        bool isarmed = it->armed;
        if (isarmed)
        {
            it->armed = false;
        }
        else
        {
            it->armed = true;
        }
    }
}

void CameraHandler::changeScalefactor(double value, const QString &cameraName)
{
    auto it = std::find_if(cameras.begin(), cameras.end(), [cameraName](const CameraInfo &camera) {
        return camera.cameraname == cameraName;
    });

    if (it != cameras.end())
    {
        it->scaleFactor = value;
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

QVector<QPair<QDate, QPair<cv::Mat, QTime>>> CameraHandler::getFrameBuffer(const QString& cameraname) const {
    auto it = std::find_if(cameras.begin(), cameras.end(), [cameraname](const CameraInfo& camera) {
        return camera.cameraname == cameraname;
    });

    if (it != cameras.end()) {
        return it->CameraRecording;
    }

    return QVector<QPair<QDate, QPair<cv::Mat, QTime>>>(); // Return empty buffer if not found
}

void CameraHandler::clearFrameBuffer(const QString& cameraname) {
    auto it = std::find_if(cameras.begin(), cameras.end(), [cameraname](const CameraInfo& camera) {
        return camera.cameraname == cameraname;
    });

    if (it != cameras.end()) {
        it->frameBuffer.clear();
    }
}

void CameraHandler::queueSerializationTask(CameraInfo &camera)
{
    // Queue a task to serialize the frame buffer data
    QThreadPool::globalInstance()->start([=]() {
        serialize(camera);
    });
}

void serializeMat(QDataStream &stream, const cv::Mat &mat)
{
    // Convert the cv::Mat object to a QByteArray for serialization
    QByteArray matData;
    QDataStream dataStream(&matData, QIODevice::WriteOnly);
    dataStream << mat.cols << mat.rows << mat.type() << QByteArray((char*)mat.data, mat.total() * mat.elemSize());

    // Serialize the QByteArray containing cv::Mat data
    stream << matData;
}


void deserializeMat(QDataStream &stream, cv::Mat &mat)
{
    // Deserialize the cv::Mat object from the QByteArray
    QByteArray matData;
    stream >> matData;

    QDataStream dataStream(matData);
    int cols, rows, type;
    dataStream >> cols >> rows >> type;

    mat = cv::Mat(rows, cols, type);
    dataStream.readRawData((char*)mat.data, mat.total() * mat.elemSize());
}


void CameraHandler::serialize(const CameraInfo &camera)
{
    QFile file(camera.cameraname + ".dat");
    if (file.open(QIODevice::WriteOnly)) {
        QDataStream out(&file);

        // Serialize the number of frames in the frame buffer
        out << camera.CameraRecording.size();

        // Serialize each frame (cv::Mat object) and its corresponding QTime
        for (const auto& framePair : camera.CameraRecording) {
            out << framePair.first;

            // Serialize the QTime
            out << framePair.second.second;

            // Serialize the cv::Mat object
            serializeMat(out, framePair.second.first);
        }

        file.close();
    }
    else
    {
        qDebug() << "Error opening file for writing:" << file.errorString();
    }
}

void CameraHandler::deserialize(CameraInfo &camera)
{
    QFile file(camera.cameraname + ".dat");
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);
        // Get the size of the file for progress calculation
        qint64 fileSize = file.size();

        // Initialize progress indicator
        QProgressDialog progressDialog("Deserializing...", "Cancel", 0, 100);
        progressDialog.setWindowModality(Qt::WindowModal);
        progressDialog.setMinimumDuration(0); // Show progress dialog immediately

        // Clear existing data in the CameraRecording buffer
        camera.CameraRecording.clear();

        // Deserialize the number of frames in the frame buffer
        int numFrames;
        in >> numFrames;

        qint64 bytesRead = sizeof(numFrames);

        // Deserialize each frame (cv::Mat object) and its corresponding QTime
        for (int i = 0; i < numFrames; ++i) {
            if (progressDialog.wasCanceled())
                break;

            QDate date;
            in >> date;
            bytesRead += sizeof(date);

            QTime time;
            in >> time;
            bytesRead += sizeof(time);

            cv::Mat frame;
            deserializeMat(in, frame);
            bytesRead += frame.total() * frame.elemSize();
            qDebug() << bytesRead;

            // Append the frame and its corresponding date to the CameraRecording buffer
            camera.CameraRecording.append(qMakePair(date, qMakePair(frame, time)));

            // Update progress
            int progress = static_cast<int>((bytesRead * 100) / fileSize);
            progressDialog.setValue(progress);
        }

        file.close();
        progressDialog.setValue(100); // Ensure progress reaches 100% at the end
    }
    else {
        qDebug() << "Error opening file for reading:" << file.errorString();
    }
}
