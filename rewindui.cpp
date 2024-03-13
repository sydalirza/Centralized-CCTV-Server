// rewindui.cpp
#include "rewindui.h"
#include "ui_rewindui.h"
#include <QThread>

RewindUi::RewindUi(const QString& cameraName, const QVector<QPair<QImage, QTime>>& frameBuffer, QWidget* parent)
    : QWidget(parent), ui(new Ui::RewindUi), isPlaying(false), currentFrameIndex(0), frameBuffer(frameBuffer), cameraname(cameraName) {
    ui->setupUi(this);

    // Connect signals to slots`
    connect(ui->play_button, &QPushButton::clicked, this, &RewindUi::onPlayButtonClicked);
    connect(ui->pause_button, &QPushButton::clicked, this, &RewindUi::onPauseButtonClicked);
    connect(ui->horizontalSlider, &QSlider::valueChanged, this, &RewindUi::onSliderValueChanged);

    QTime lastFrameTime = frameBuffer.last().second;

    ui->camera_name->setText(cameraname);

    // Update label_2 with the time of the last frame
    ui->label_2->setText(lastFrameTime.toString());

    // Set initial values
    ui->horizontalSlider->setRange(0, frameBuffer.size() - 1);
    ui->horizontalSlider->setValue(0);

    this->moveToThread(&workerThread);

    // Connect signals and slots for thread management
    connect(&workerThread, &QThread::finished, this, &RewindUi::deleteLater);
    connect(&workerThread, &QThread::finished, &workerThread, &QThread::deleteLater);

    // Start the thread
    workerThread.start();
}

RewindUi::~RewindUi() {
    workerThread.quit();
    workerThread.wait();

    delete ui;
}

void RewindUi::onPlayButtonClicked() {
    isPlaying = true;
    // Start playing frames
    while (isPlaying && currentFrameIndex < frameBuffer.size()) {
        // Play frame at currentFrameIndex
        updateUIFromFrame(currentFrameIndex);

        QCoreApplication::processEvents(); // Allow UI to update
        // Move to the next frame
        ++currentFrameIndex;
    }
}

void RewindUi::onPauseButtonClicked() {
    isPlaying = false;
}

void RewindUi::onSliderValueChanged(int value) {
    // Handle slider value change
    currentFrameIndex = value;
    updateUIFromFrame(currentFrameIndex);
}

void RewindUi::updateUIFromFrame(int frameIndex) {
    // Update UI elements based on the frame at frameIndex
    QImage frameImage = frameBuffer[frameIndex].first;
    QTime currentTime = frameBuffer[frameIndex].second;

    // Display the frame image (assuming you have a QLabel named video_display)
    ui->video_display->setPixmap(QPixmap::fromImage(frameImage).scaled(ui->video_display->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    // Update labels with current time information
    ui->label->setText(currentTime.toString());

    // Update slider position
    ui->horizontalSlider->setValue(frameIndex);
}

void RewindUi::on_goto_start_clicked()
{
    currentFrameIndex = 0;
    updateUIFromFrame(currentFrameIndex);
}


void RewindUi::on_goto_end_clicked()
{
    currentFrameIndex = frameBuffer.size() - 1;
    updateUIFromFrame(currentFrameIndex);
}

void RewindUi::on_save_recording_clicked()
{
    saving_recording = !saving_recording;

    if(saving_recording)
    {
        startFrameindex = currentFrameIndex;

        QTime currentTime = frameBuffer[startFrameindex].second;

        ui->from_time->setTime(currentTime);
    }
    else
    {
        endFrameindex = currentFrameIndex;

        QTime currentTime = frameBuffer[endFrameindex].second;

        ui->till_time->setTime(currentTime);

        saveRecording(startFrameindex, endFrameindex);
    }
}


Mat imageToMat(const QImage& image) {
    // Convert QImage to cv::Mat with Format_RGB32
    QImage convertedImage = image.convertToFormat(QImage::Format_RGB32);

    // Convert QImage to cv::Mat
    return cv::Mat(convertedImage.height(), convertedImage.width(), CV_8UC4, const_cast<uchar*>(convertedImage.bits()), convertedImage.bytesPerLine()).clone();
}

void RewindUi::saveRecording(int startFrameindex, int endFrameindex)
{
    qDebug() << "Saving your recording from " << frameBuffer[startFrameindex].second.toString() << " till " << frameBuffer[endFrameindex].second.toString();

    QString fileName = QString("%1_%2_%3.avi")
                           .arg(cameraname)
                           .arg(frameBuffer[startFrameindex].second.toString("hhmmss"))
                           .arg(frameBuffer[endFrameindex].second.toString("hhmmss"));

    VideoWriter videoWriter(fileName.toStdString(), VideoWriter::fourcc('X', 'V', 'I', 'D'), 10, Size(CAP_PROP_FRAME_WIDTH, CAP_PROP_FRAME_HEIGHT));

    // Iterate through the frames in the specified range and write them to the VideoWriter
    for (int i = startFrameindex; i <= endFrameindex; ++i) {
        QImage frameImage = frameBuffer[i].first;
        Mat frameMat = imageToMat(frameImage); // Assuming you have a function to convert QImage to cv::Mat
        videoWriter.write(frameMat);
    }

    // Release the VideoWriter
    videoWriter.release();
}

