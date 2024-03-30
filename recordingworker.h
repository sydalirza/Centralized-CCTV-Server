#ifndef RECORDINGWORKER_H
#define RECORDINGWORKER_H

#include <QString>
#include <QTime>
#include <opencv2/opencv.hpp>

using namespace cv;


class RecordingWorker: public QObject
{
    Q_OBJECT
public:
    RecordingWorker();

    void recordvideo(int startFrameindex, int endFrameindex, const QString &cameraname, const QVector<QPair<QDate, QPair<Mat, QTime>>> &frameBuffer);
};

#endif // RECORDINGWORKER_H
