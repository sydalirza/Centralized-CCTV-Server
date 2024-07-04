// mainwindow.h

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include<QGridLayout>
#include <QMainWindow>
#include <QLabel>  // Add this line
#include "camerasettings.h"
#include "camerascreens.h"
#include "faceshandler.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:

private slots:

    void on_tabWidget_tabCloseRequested(int index);

    void openDefaultTab();

    bool tab_already_open(const QString &tabname);

    void on_tab_button_1_clicked();

    void setMaxSizeBasedOnScreen();

    void on_close_button_clicked();

    void on_cameras_button_clicked();

    void update_camera_buttons(const std::pair<QString, QString> camera);

    void remove_camera_button(const QString &cameraName);

    std::vector<std::pair<QString, QString>> getAllCameras();

    void on_one_layout_clicked();

    void on_four_layout_clicked();

    void on_sixteen_layout_clicked();

    void updateDateTime();

    void on_settings_button_clicked();

    void add_new_face(dlib::matrix<float, 0, 1> face_encoding);

    void hide_close_button(int tabIndex);

    void on_loadfaceencodings_button_clicked();

private:
    Ui::MainWindow *ui;

    CameraSettings *cameraSettingsInstance = nullptr;
    CameraScreens *cameraScreens = nullptr;
    faceshandler *facesHandlerInstance = nullptr;
    std::vector<std::pair<QString, QString>> cameras;

};

#endif // MAINWINDOW_H
