#include "recordingworker.h"

RecordingWorker::RecordingWorker() {

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

// Implementation for the metaObject function
const QMetaObject *RecordingWorker::metaObject() const {
    return QObject::metaObject();
}

// Implementation for the qt_metacast function
void *RecordingWorker::qt_metacast(const char *className) {
    return QObject::qt_metacast(className);
}

// Implementation for the qt_metacall function
int RecordingWorker::qt_metacall(QMetaObject::Call call, int id, void **arguments) {
    return QObject::qt_metacall(call, id, arguments);
}
