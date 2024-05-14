// rewindui.cpp
#include "rewindui.h"
#include "ui_rewindui.h"
#include <QThread>

RewindUi::RewindUi(const QString& cameraName, const QVector<QPair<QDate, QPair<Mat, QTime>>>& frameBuffer, QWidget* parent)
    : QWidget(parent), ui(new Ui::RewindUi), isPlaying(false), currentFrameIndex(0), frameBuffer(frameBuffer), cameraname(cameraName) {
    ui->setupUi(this);

    // Connect signals to slots`
    connect(ui->play_button, &QPushButton::clicked, this, &RewindUi::onPlayButtonClicked);
    connect(ui->pause_button, &QPushButton::clicked, this, &RewindUi::onPauseButtonClicked);
    connect(ui->horizontalSlider, &QSlider::valueChanged, this, &RewindUi::onSliderValueChanged);

    ui->play_button->setIcon(QIcon("Icons/play.png"));
    ui->pause_button->setIcon(QIcon("Icons/pause.png"));
    ui->goto_end->setIcon(QIcon("Icons/next.png"));
    ui->goto_start->setIcon(QIcon("Icons/prev.png"));


    // Disable UI elements initially
    ui->play_button->setEnabled(false);
    ui->pause_button->setEnabled(false);
    ui->horizontalSlider->setEnabled(false);

    QTime lastFrameTime = frameBuffer.last().second.second;

    ui->camera_name->setText(cameraname);

    // Update label_2 with the time of the last frame
    ui->label_2->setText(lastFrameTime.toString());

    QSet<QDate> uniqueDates;

    // Add the dates from frameBuffer
    for (const auto& entry : frameBuffer) {
        uniqueDates.insert(entry.first);
    }

    // Populate the combobox with unique dates
    for (const auto& date : uniqueDates) {
        ui->date->addItem(date.toString(Qt::ISODate), QVariant(date));
    }

    // Connect signals and slots for thread management
    connect(&workerThread, &QThread::finished, this, &RewindUi::deleteLater);
    connect(&workerThread, &QThread::finished, &workerThread, &QThread::deleteLater);
}

RewindUi::~RewindUi() {
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
    Mat frameMat = frameBuffer[frameIndex].second.first;
    QTime currentTime = frameBuffer[frameIndex].second.second;


    // Display the frame image (assuming you have a QLabel named video_display)
    ui->video_display->setPixmap(QPixmap::fromImage(matToImage(frameMat)).scaled(ui->video_display->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

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

        QTime currentTime = frameBuffer[startFrameindex].second.second;

        ui->from_time->setTime(currentTime);

        ui->save_recording->setText("Stop Recording");
    }
    else
    {
        endFrameindex = currentFrameIndex;

        QTime currentTime = frameBuffer[endFrameindex].second.second;

        ui->till_time->setTime(currentTime);

        ui->save_recording->setText("Saving Recording");

        onPauseButtonClicked();

        if((frameBuffer.size() - 1) < endFrameindex || startFrameindex > endFrameindex)
        {
            qDebug() << "List index out of range or incorrect labels";
            ui->save_recording->setText("Start Recording");

        }
        else
        {
            RecordingWorker* worker = new RecordingWorker;

            // Move the worker object to a separate thread
            // Create a new thread
            // Create a new thread
            QThread* recordingThread = new QThread;

            // Move the RecordingWorker instance to the new thread
            worker -> moveToThread(recordingThread);

            // Call recordvideo from the new thread using lambda function
            QObject::connect(recordingThread, &QThread::started, [=]() {
                worker -> recordvideo(startFrameindex, endFrameindex, cameraname, frameBuffer);
            });

            // Connect thread's finished signal to deleteLater() slot to clean up when the thread finishes
            QObject::connect(recordingThread, &QThread::finished, recordingThread, &QThread::deleteLater);

            // Start the thread
            recordingThread->start();

            ui->save_recording->setText("Start Recording");
        }
        }
}

QImage RewindUi::matToImage(Mat& mat)
{
    if (mat.channels() == 3) {
        QImage image(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        return image.rgbSwapped();
    } else if (mat.channels() == 1) {
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8);
    }

    qDebug() << "Unsupported Image Format";
    return QImage();
}

void RewindUi::on_date_currentIndexChanged(int index)
{
    if (index < 0 || index >= ui->date->count()) {
        // Invalid index, disable UI elements and return
        ui->play_button->setEnabled(false);
        ui->pause_button->setEnabled(false);
        ui->horizontalSlider->setEnabled(false);
        return;
    }

    // Get the selected date from the combobox
    QDate selectedDate = ui->date->itemData(index).toDate();

    // Initialize first and last frame indexes
    int firstFrameIndex = -1;
    int lastFrameIndex = -1;

    // Find the indexes of the first and last frames for the selected date
    for (int i = 0; i < frameBuffer.size(); ++i) {
        if (frameBuffer[i].first == selectedDate) {
            if (firstFrameIndex == -1) {
                firstFrameIndex = i;
            }
            lastFrameIndex = i;
        }
    }

    if (firstFrameIndex != -1 && lastFrameIndex != -1) {
        // Display frames for the selected date
        qDebug() << "Frames found from this date";
        updateUIFromFrame(firstFrameIndex);
        ui->horizontalSlider->setRange(firstFrameIndex, lastFrameIndex);
        ui->horizontalSlider->setValue(firstFrameIndex);

        QTime lastFrameTime = frameBuffer[lastFrameIndex].second.second;

        // Update label_2 with the time of the last frame
        ui->label_2->setText(lastFrameTime.toString());

        // Enable UI elements
        enableeverything();
    }
    else {
        // No frames found for the selected date, disable UI elements
        qDebug() << "No frames found from this date";
        disableeverything();

    }
}

void RewindUi::disableeverything()
{
    ui->play_button->setEnabled(false);
    ui->pause_button->setEnabled(false);
    ui->horizontalSlider->setEnabled(false);
    ui->goto_end->setEnabled(false);
    ui->goto_start->setEnabled(false);
    ui->video_display->clear();
    ui->label->clear();
    ui->label_2->clear();
    ui->horizontalSlider->setRange(0, 0);
    ui->horizontalSlider->setValue(0);
}

void RewindUi::enableeverything()
{
    // Enable UI elements
    ui->play_button->setEnabled(true);
    ui->pause_button->setEnabled(true);
    ui->horizontalSlider->setEnabled(true);
    ui->goto_end->setEnabled(true);
    ui->goto_start->setEnabled(true);

}
