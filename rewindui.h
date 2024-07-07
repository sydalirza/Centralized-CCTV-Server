// rewindui.h
#ifndef REWINDUI_H
#define REWINDUI_H

#include <QWidget>
#include <QImage>
#include <QTime>
#include <QThread>
#include <QTimer>
#include <opencv2/opencv.hpp>
#include "recordingworker.h"

using namespace cv;

namespace Ui {
class RewindUi;
}

class RewindUi : public QWidget {
    Q_OBJECT

public:
    explicit RewindUi(const QString& cameraname, const QVector<QPair<QDate, QPair<Mat, QTime>>>& frameBuffer, QWidget* parent = nullptr);
    ~RewindUi();

public slots:
    void onPlayButtonClicked();
    void onPauseButtonClicked();
    void onSliderValueChanged(int value);

private slots:
    void on_goto_start_clicked();

    void on_goto_end_clicked();

    void on_save_recording_clicked();

    void on_date_currentIndexChanged(int index);

    void on_cancel_recording_clicked();

private:
    Ui::RewindUi* ui;
    bool isPlaying;
    int currentFrameIndex;
    double playbackSpeed = 1.0;
    QVector<QPair<QDate, QPair<Mat, QTime>>> frameBuffer;
    QString cameraname;

    void updateFrame();
    void updateUIFromFrame(int frameIndex);
    void saveRecording(int startFrameindex, int endFrameindex);
    void disableeverything();
    void enableeverything();

    QImage matToImage(Mat &mat);

    int startFrameindex;
    int endFrameindex;
    bool saving_recording = false;

    QThread workerThread;
    RecordingWorker recordWorker;

    QTimer *playbackTimer;
};

#endif // REWINDUI_H
