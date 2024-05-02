#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>
#include <QMimeData>
#include <QDrag>
#include <QVBoxLayout>
#include <QPushButton>
#include <QGridLayout>
#include <QFileDialog>
#include <QPixmap>
#include <QLabel>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDesktopServices>
#include <QTabBar>
#include <camerascreens.h>
#include <camerasettings.h>
#include <QScreen>
#include <QGuiApplication>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    openDefaultTab();
    setFixedSize(QGuiApplication::primaryScreen()->availableSize());
    cameraScreens = qobject_cast<CameraScreens*>(ui->tabWidget->currentWidget());


    cameraSettingsInstance = new CameraSettings(); // Create an instance of CameraSettings
    connect(cameraSettingsInstance, &CameraSettings::add_camera, this, &MainWindow::update_camera_buttons);
    connect(cameraSettingsInstance, &CameraSettings::delete_camera, this, &MainWindow::remove_camera_button);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setMaxSizeBasedOnScreen()
{
    QSize screenSize = QGuiApplication::primaryScreen()->availableSize();
    this->setMaximumSize(screenSize);

    // Set the initial size to be smaller than the screen size
    // this->resize(screenSize.width() * 0.8, screenSize.height() * 0.8);
}

void MainWindow::on_tabWidget_tabCloseRequested(int index)
{
    ui -> tabWidget -> removeTab(index);
}

void MainWindow::on_tab_button_1_clicked()
{
    ui -> tabWidget -> addTab(new CameraScreens(), "New Tab");
}

void MainWindow::openDefaultTab()
{
    const vector<std::pair<QString, QString>> cameras = {
        // {"rtsp://192.168.1.9/live/ch00_0", "Garage"}
        {"3.mp4", "Camera 1"},
        {"4.mp4", "Camera 2"}
        /*{"rtsp://10.4.72.198:8080/h264.sdp", "Camera 2"}*/
        // {"rtsp://192.168.1.4:8080/h264.sdp", "Camera 2"}
    };

    for (const auto& camera : cameras){
        update_camera_buttons(camera);
    }

    CameraScreens *defaultTab = new CameraScreens(this, this);
    int tabIndex = ui->tabWidget->addTab(defaultTab, "Main View");
    QTabBar* tabBar = ui->tabWidget->findChild<QTabBar*>();
    if (tabBar)
    {
        QWidget* closeButton = tabBar->tabButton(tabIndex, QTabBar::RightSide);
        if (closeButton)
        {
            closeButton->resize(0, 0);
            closeButton->setVisible(false);
        }
    }
}

void MainWindow::update_camera_buttons(const std::pair<QString, QString> camera)
{
    QVBoxLayout *cameraLayout = qobject_cast<QVBoxLayout*>(ui->cameranames->layout());
    if (cameraLayout) {
        // Add the camera buttons to the existing layout
        QPushButton *cameraButton = new QPushButton(camera.second);
        cameraLayout->addWidget(cameraButton);

            // Connect the button's clicked signal to open the corresponding camera tab
        connect(cameraButton, &QPushButton::clicked, this, [=]()
        {
            // CameraScreens *cameraScreens = qobject_cast<CameraScreens*>(ui->tabWidget->currentWidget());
                if (cameraScreens) {
                    cameraScreens->addCamera(camera.first, camera.second);
                }
            });
        }
    else {
        qDebug() << "Error: cameranames layout is not a QVBoxLayout";
    }
}

void MainWindow::on_close_button_clicked()
{
    qApp->quit();
}


void MainWindow::on_cameras_button_clicked()
{
    if(!tab_already_open("Cameras"))
    {
        ui -> tabWidget -> addTab(cameraSettingsInstance, "Cameras");
    }
    else
    {
        qDebug() << "Tab Already Open";
    }
}

bool MainWindow::tab_already_open(const QString &tabname)
{
    bool tabFound = false;
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        if (ui->tabWidget->tabText(i) == tabname) {
            tabFound = true;
            break;
        }
    }

    return tabFound;

}

void MainWindow::remove_camera_button(const QString &cameraName)
{
    QVBoxLayout *cameraLayout = qobject_cast<QVBoxLayout*>(ui->cameranames->layout());
    if (cameraLayout) {
        if (cameraScreens) {
            cameraScreens->removeCamera(cameraName);
        }
        // Find the button corresponding to the camera name and remove it from the layout
        for (int i = 0; i < cameraLayout->count(); ++i) {
            QPushButton *button = qobject_cast<QPushButton*>(cameraLayout->itemAt(i)->widget());
            if (button && button->text() == cameraName) {
                cameraLayout->removeWidget(button);
                delete button;
                break; // Assuming each camera name is unique, so we can exit the loop after finding and removing the button
            }
        }
    } else {
        qDebug() << "Error: cameranames layout is not a QVBoxLayout";
    }
}
