// rewindui.h
#ifndef REWINDUI_H
#define REWINDUI_H

#include <QWidget>
#include <QImage>
#include <QTime>
#include <QThread>
#include <opencv2/opencv.hpp>

using namespace cv;

namespace Ui {
class RewindUi;
}

class RewindUi : public QWidget {
    Q_OBJECT

public:
    explicit RewindUi(const QString& cameraname, const QVector<QPair<QImage, QTime>>& frameBuffer, QWidget* parent = nullptr);
    ~RewindUi();

public slots:
    void onPlayButtonClicked();
    void onPauseButtonClicked();
    void onSliderValueChanged(int value);

private slots:
    void on_goto_start_clicked();

    void on_goto_end_clicked();

    void on_save_recording_clicked();

private:
    Ui::RewindUi* ui;
    bool isPlaying;
    int currentFrameIndex;
    double playbackSpeed = 1.0;
    QVector<QPair<QImage, QTime>> frameBuffer;
    QString cameraname;

    void updateUIFromFrame(int frameIndex);
    void saveRecording(int startFrameindex, int endFrameindex);

    int startFrameindex;
    int endFrameindex;
    bool saving_recording = false;

    QThread workerThread;
};

#endif // REWINDUI_H
