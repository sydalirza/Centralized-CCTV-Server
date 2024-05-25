#include "recordingworker.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>

RecordingWorker::RecordingWorker() {

}

void RecordingWorker::recordvideo(int startFrameindex, int endFrameindex, const QString &cameraname, const QVector<QPair<QDate, QPair<Mat, QTime>>>& frameBuffer, QSqlDatabase db)
{
    // Check if the frame indexes are valid
    if (startFrameindex < 0 || endFrameindex >= frameBuffer.size() || startFrameindex > endFrameindex) {
        qDebug() << "Invalid frame indexes for recording.";
        return;
    }

    // Create the directory for the camera if it doesn't exist
    QString cameraDirPath = "Recordings/" + cameraname;
    QDir cameraDir(cameraDirPath);
    if (!cameraDir.exists()) {
        if (!cameraDir.mkpath(cameraDirPath)) {
            qDebug() << "Error: Failed to create directory for camera: " << cameraname;
            return;
        }
    }

    // Create VideoWriter object
    QString fileName = QString("%1_%2_%3_%4.mp4")
                           .arg(cameraname)
                           .arg(frameBuffer[startFrameindex].first.toString())
                           .arg(frameBuffer[startFrameindex].second.second.toString("hhmmss"))
                           .arg(frameBuffer[endFrameindex].second.second.toString("hhmmss"));

    QString filePath = cameraDirPath + "/" + fileName;

    VideoWriter videoWriter(filePath.toStdString(), VideoWriter::fourcc('M', 'P', '4', 'V'), 30, frameBuffer[startFrameindex].second.first.size());

    // Check if VideoWriter is opened successfully
    if (!videoWriter.isOpened()) {
        qDebug() << "Error opening VideoWriter for recording:" << filePath;
        return;
    }

    // Write frames to the video file
    for (int i = startFrameindex; i <= endFrameindex; ++i) {
        videoWriter.write(frameBuffer[i].second.first);
    }

    // Release VideoWriter resources
    videoWriter.release();

    // Insert log into database
    QSqlQuery query(db); // Pass the database connection to QSqlQuery constructor
    query.prepare("INSERT INTO camera_logs (camera_name, file_name, start_time, end_time) VALUES (:camera_name, :file_name, :start_time, :end_time)");
    query.bindValue(":camera_name", cameraname);
    query.bindValue(":file_name", filePath);
    query.bindValue(":start_time", frameBuffer[startFrameindex].second.second.toString("hh:mm:ss"));
    query.bindValue(":end_time", frameBuffer[endFrameindex].second.second.toString("hh:mm:ss"));
    if (!query.exec()) {
        qDebug() << "Error inserting log into database:" << query.lastError().text();
    } else {
        qDebug() << "Video recording saved: " << filePath;
    }
}

void RecordingWorker::recordvideo(int startFrameindex, int endFrameindex, const QString &cameraname, const QVector<QPair<QDate, QPair<Mat, QTime>>>& frameBuffer)
{
    // Check if the frame indexes are valid
    if (startFrameindex < 0 || endFrameindex >= frameBuffer.size() || startFrameindex > endFrameindex) {
        qDebug() << "Invalid frame indexes for recording.";
        return;
    }

    // Create VideoWriter object
    QString fileName = QString("%1_%2_%3_%4.avi")
                           .arg(cameraname)
                           .arg(frameBuffer[startFrameindex].first.toString())
                           .arg(frameBuffer[startFrameindex].second.second.toString("hhmmss"))
                           .arg(frameBuffer[endFrameindex].second.second.toString("hhmmss"));

    VideoWriter videoWriter(fileName.toStdString(), VideoWriter::fourcc('M', 'J', 'P', 'G'), 30, frameBuffer[startFrameindex].second.first.size());

    // Check if VideoWriter is opened successfully
    if (!videoWriter.isOpened()) {
        qDebug() << "Error opening VideoWriter for recording.";
        return;
    }

    // Write frames to the video file
    for (int i = startFrameindex; i <= endFrameindex; ++i) {
        videoWriter.write(frameBuffer[i].second.first);
    }

    // Release VideoWriter resources
    videoWriter.release();

    qDebug() << "Video recording saved: " << fileName;
}
