#ifndef CAMERASCREENS_H
#define CAMERASCREENS_H

#include <QWidget>
#include <QMessageBox>
#include <QTabWidget>
#include "customlabel.h"
#include "camerahandler.h"
#include "camerasettings.h"

namespace Ui {
class CameraScreens;
}

class CameraScreens : public QWidget
{
    Q_OBJECT
    static const int MAX_RECONNECT_ATTEMPTS = 3;



public:
    explicit CameraScreens(QWidget *parent = nullptr, QWidget *parentWidget = nullptr, const std::vector<std::pair<QString, QString>> &cameras = {});

    void connectCameras();
    ~CameraScreens();

signals:
    void add_new_face(dlib::matrix<float, 0, 1> face_encoding);

public slots:
    void addCamera(const QString& cameraUrl, const QString& cameraName);
    void removeCamera(const QString& cameraName);
    void on_one_camera_clicked();
    void on_four_camera_clicked();
    void on_sixteen_camera_clicked();

private slots:

    void onImageClicked();

    void onImageDoubleClicked();

    void handleFrameUpdate(const QImage &frame, const QString &cameraname);

    void initialize();

    void handleCameraOpened();

    void handleCameraClosed();

    void handleCameraOpeningFailed(const QString &cameraName);

    void changeCamerastatus(const QString &cameraName);

    void on_scale_factor_slider_valueChanged(int value, const QString &cameraName);

    void handleTabCloseRequested(int index);

private:
    Ui::CameraScreens *ui;
    QTimer *timer;
    QTabWidget* tabWidget;

    // Function to dynamically update the camera layout based on the number of connected cameras
    void updateCameraLayout(int numberOfConnectedCameras, int total_screens);

    // Assume you have a function to get the number of connected cameras
    int getNumberOfConnectedCameras();

    CustomLabel* lastClickedLabel = nullptr;
    CustomLabel *selectedLabel;

    QWidget* parentWidget;

    int totalWalls = 1; // Set an initial value, adjust as needed
    int currentWall = 16; // Set an initial value, adjust as needed
    int camerasPerWall = 16; // Set an initial value, adjust as needed

    const std::vector<std::pair<QString, QString>> cameras;
    QVector<QLabel*> cameraLabels;  // Keep track of the QLabel widgets
    CameraHandler cameraHandler;    // Instance of CameraHandler
    CameraSettings cameraSettings;
    QMap<QString, CustomLabel*> cameraLabelMap;
    void addCameraLabel(const QString &cameraname, int total_screens, int i);
};

#endif // CAMERASCREENS_H
