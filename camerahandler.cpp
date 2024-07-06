#include "camerahandler.h"
#include "recordingworker.h"
#include "dlib_utils.h"

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

using namespace dlib;

CameraHandler:: CameraHandler(QObject *parent) : QObject(parent), timer(new QTimer(this)), encodings(*new std::vector<dlib::matrix<float, 0, 1>>())
{
    connect(timer, &QTimer::timeout, this, &CameraHandler::updateFrames);
    timer->start(30); //FPS

    // Connect cleanup timer to cleanupOldFrames method
    connect(&cleanupTimer, &QTimer::timeout, this, &CameraHandler::cleanupOldFrames);
    cleanupTimer.start(24 * 60 * 60 * 1000); // Runs once every day

    std::string faceClassifier = "haarcascade_frontalface_alt2.xml";

    if (!faceCascade.load(faceClassifier)) {
        qDebug() << "Could not load the classifier";
        QCoreApplication::exit(-1);
    }

    qDebug() << "Classifier Loaded!";

    // recognizer->read("trained_model.yml");

    initialize_network();
    initialize_shape_predictor();
    load_face_encodings("encode");


    db = QSqlDatabase::addDatabase("QSQLITE", "cameras_connection");
    db.setDatabaseName("cameras.db");

    // Opening the database connection
    if (!db.open()) {
        qDebug() << "Error: Failed to open the database.";
    } else {
        // Creates the 'camera_logs' table if it doesn't exist
        QSqlQuery query(db); // Passing the database connection to QSqlQuery constructor
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

void CameraHandler::load_face_encodings(const std::string& folder_path)
{
    // Initializing Dlib face detector, shape predictor, and face recognition model
    frontal_face_detector detector = get_frontal_face_detector();

    // Iterate through all images in the folder
    std::vector<cv::String> filenames;
    cv::glob(folder_path, filenames);

    for (const auto& filename : filenames)
    {
        // Read the image using OpenCV
        cv::Mat image = cv::imread(filename);

        // Convert OpenCV image to Dlib's format
        dlib::cv_image<dlib::bgr_pixel> dlib_img(image);

        // Detect faces in the image
        std::vector<dlib::rectangle> faces = detector(dlib_img);

        // Check if exactly one face is detected
        if (faces.size() != 1)
        {
            std::cerr << "Error: Expected exactly one face in " << filename << ", but found " << faces.size() << " faces." << std::endl;
            continue;
        }

        // Get the shape (facial landmarks) of the detected face
        dlib::full_object_detection shape = sp(dlib_img, faces[0]);

        // Generate the face encoding using Dlib's face recognition model
        dlib::matrix<dlib::rgb_pixel> face_chip;
        dlib::extract_image_chip(dlib_img, dlib::get_face_chip_details(shape, 150, 0.25), face_chip);
        dlib::matrix<float, 0, 1> face_encoding = net(face_chip);

        // Store the face encoding
        encodings.push_back(face_encoding);

        // Print the filename and the corresponding encoding
        std::cout << "Filename: " << filename << std::endl;
        std::cout << "Face Encoding: " << face_encoding << std::endl;
    }
}

void CameraHandler::closeAllCameras()
{
    for (auto& camera : cameras)
    {
        CloseCamera(camera.cameraname);
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

            // QString filePath = cameraname + ".dat";
            // if (QFile::exists(filePath)) {
            //     // Ask the user if they want to reload the camera
            //     QMessageBox::StandardButton reply;
            //     reply = QMessageBox::question(nullptr, "Reload Camera", "A serialized file for this camera already exists. Do you want to reload the camera from the serialized file?",
            //                                   QMessageBox::Yes|QMessageBox::No);
            //     if (reply == QMessageBox::Yes) {
            //         // User chose not to reload, so just return
            //         deserialize(newcamera);
            //     }
            // }

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

    // If the reconnection was successful, set the flag to false
    if (camera.videoCapture.isOpened()) {
        qDebug() << "Reconnected " << camera.cameraname;
        camera.isError = false;
        camera.isReconnecting = false;
    }

    // If all reconnection attempts failed, remove the camera
    if (camera.isError) {
        qDebug() << "Removing " << camera.cameraname << " due to disconnection";
        CloseCamera(camera.cameraname);
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
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString formattedDateTime = currentDateTime.toString("yyyy-MM-dd hh:mm:ss.zzz");

    double fontSize = 0.5 * camera.scaleFactor;
    int thickness = static_cast<int>(1 * camera.scaleFactor);  // Adjust thickness based on scale factor

    // Put the date and timestamp on the frame
    cv::putText(resizedFrame, formattedDateTime.toStdString(), cv::Point(10, resizedFrame.rows - 10), cv::FONT_HERSHEY_SIMPLEX, fontSize, cv::Scalar(255, 255, 255), thickness);

    if(!camera.armed)
    {
        return resizedFrame;
    }

    // Set confidence threshold
    const double confidenceThreshold = 0.7; // Adjust this value as needed

    cv::Mat frame_gray;
    cvtColor(resizedFrame, frame_gray, cv::COLOR_BGR2GRAY);

    std::vector<cv::Rect> faces;
    double scaleFactor = 1.3; // Experiment with different values (e.g., 1.1, 1.2, etc.)
    int minNeighbors = 1; // Experiment with different values (e.g., 3, 5, 7, etc.)
    int flags = cv::CASCADE_SCALE_IMAGE;
    faceCascade.detectMultiScale(frame_gray, faces, scaleFactor, minNeighbors, flags);

    bool match_found = false;
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
        for (auto face : faces)
        {
            // Convert OpenCV rect to dlib rect
            dlib::rectangle dlibFaceRect(face.x, face.y, face.x + face.width, face.y + face.height);

            dlib::cv_image<unsigned char> cimg(frame_gray);
            // Find the landmarks using the 5 landmarks model
            dlib::full_object_detection shape = sp(cimg, dlibFaceRect);
            // Extract the face chip
            dlib::matrix<dlib::rgb_pixel> face_chip;
            extract_image_chip(cimg, get_face_chip_details(shape, 150, 0.25), face_chip);
            // Get the face encoding
            dlib::matrix<float, 0, 1> face_encoding = net(face_chip);

            // Compare this face encoding with the known faces
            double min_distance = confidenceThreshold; // Threshold for recognizing a face
            for (const auto& known_encoding : encodings) {
                double distance = length(face_encoding - known_encoding);
                if (distance < min_distance) {
                    match_found = true;
                    break;
                }
            }

            // Draw a rectangle and label on the face
            if (match_found) {
                cv::rectangle(resizedFrame, face, cv::Scalar(0, 255, 0), 1);
            } else {
                cv::rectangle(resizedFrame, face, cv::Scalar(0, 0, 255), 1);
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
                if (!match_found)
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
                else
                {
                    qDebug() << "Recognized Person";
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

    // queueSerializationTask(camera);
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

void CameraHandler::add_new_face(dlib::matrix<float, 0, 1> face_encoding)
{
    qDebug()<< "Face Added!";
    encodings.push_back(face_encoding);
}

void CameraHandler::delete_face(int num)
{
    if (num >= 0 && static_cast<size_t>(num) < encodings.size()) {
        encodings.erase(encodings.begin() + num); // Erase element at index num
        qDebug() << "Face Deleted!";
    } else {
        qDebug() << "Invalid index for deletion.";
    }
}
