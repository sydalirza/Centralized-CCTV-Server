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
#include <QSqlQuery>
#include <QSqlRecord>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    openDefaultTab();
    setFixedSize(QGuiApplication::primaryScreen()->availableSize());
    cameraScreens = qobject_cast<CameraScreens*>(ui->tabWidget->currentWidget());

    QSize iconSize(ui->one_layout->width(), ui->one_layout->height());
    ui->one_layout->setIcon(QIcon("Icons/one.png"));
    ui->one_layout->setIconSize(iconSize);
    ui->four_layout->setIcon(QIcon("Icons/four.png"));
    ui->four_layout->setIconSize(iconSize);
    ui->sixteen_layout->setIcon(QIcon("Icons/sixteen.png"));
    ui->sixteen_layout->setIconSize(iconSize);
    ui->close_button->setIcon(QIcon("Icons/close.png"));

    cameraSettingsInstance = new CameraSettings(); // Create an instance of CameraSettings
    connect(cameraSettingsInstance, &CameraSettings::add_camera, this, &MainWindow::update_camera_buttons);
    connect(cameraSettingsInstance, &CameraSettings::delete_camera, this, &MainWindow::remove_camera_button);

    // Setup the timer to update date and time
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateDateTime);
    timer->start(1000); // Update every second
}

MainWindow::~MainWindow()
{
    delete cameraScreens;
    delete ui;
}

void MainWindow::updateDateTime()
{
    // Get the current date and time and format it
    QString currentDateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    ui->date_time->setText(currentDateTime); // Assuming date_time is the name of the QLabel
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

std::vector<std::pair<QString, QString>> MainWindow::getAllCameras()
{
    // Check if the database file exists
    QString databaseName = "cameras.db";
    if (!QFile::exists(databaseName)) {
        // Database file doesn't exist, create it
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName(databaseName);

        if (!db.open()) {
            qDebug() << "Failed to create database:" << db.lastError().text();
            return cameras;
        }
        else {
            qDebug() << "Database created";
        }

        // Create the table within the database
        QString createTableQueryStr = "CREATE TABLE cameradetails ("
                                      "camera_name TEXT PRIMARY KEY NOT NULL UNIQUE, "
                                      "camera_url TEXT NOT NULL UNIQUE, "
                                      "port TEXT, "
                                      "ip_address TEXT, "
                                      "username TEXT, "
                                      "password TEXT)";
        QSqlQuery createTableQuery(createTableQueryStr);
        if (!createTableQuery.exec()) {
            qDebug() << "Failed to create table:" << createTableQuery.lastError().text();
            db.close();
            return cameras;
        }

        db.close();
    }
    else {
        // Database file exists, check if the table exists
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName(databaseName);

        if (!db.open()) {
            qDebug() << "Failed to open database:" << db.lastError().text();
            return cameras;
        }
        else {
            qDebug() << "Database opened";
        }

        QSqlQuery checkTableQuery("SELECT name FROM sqlite_master WHERE type='table' AND name='cameradetails'");
        if (!checkTableQuery.exec()) {
            qDebug() << "Failed to check table existence:" << checkTableQuery.lastError().text();
            db.close();
            return cameras;
        }

        if (!checkTableQuery.next()) {
            // cameradetails table does not exist, create it
            QString createTableQueryStr = "CREATE TABLE cameradetails ("
                                          "camera_name TEXT PRIMARY KEY NOT NULL UNIQUE, "
                                          "camera_url TEXT NOT NULL UNIQUE, "
                                          "port TEXT, "
                                          "ip_address TEXT, "
                                          "username TEXT, "
                                          "password TEXT)";
            QSqlQuery createTableQuery(createTableQueryStr);
            if (!createTableQuery.exec()) {
                qDebug() << "Failed to create table:" << createTableQuery.lastError().text();
                db.close();
                return cameras;
            }
        }

        db.close();
    }

    // Open the database
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(databaseName);

    if (!db.open()) {
        qDebug() << "Failed to open database:" << db.lastError().text();
        return cameras;
    }
    else {
        qDebug() << "Database opened";
    }

    // Execute a query to select all camera names and URLs
    QSqlQuery query("SELECT camera_name, camera_url FROM cameradetails");

    // Iterate over the result set and populate the vector of pairs
    while (query.next()) {
        QString cameraName = query.value("camera_name").toString();
        QString cameraUrl = query.value("camera_url").toString();
        cameras.push_back(std::make_pair(cameraUrl, cameraName));
    }

    // Close the database connection
    db.close();

    return cameras;
}


void MainWindow::openDefaultTab()
{
    cameras = getAllCameras();

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
    qDebug() << "Number of Tabs: " << ui->tabWidget->count();
    qDebug() << "Current Index: " << ui->tabWidget->currentIndex();
}

void MainWindow::update_camera_buttons(const std::pair<QString, QString> camera)
{
    QVBoxLayout *cameraLayout = qobject_cast<QVBoxLayout*>(ui->cameranames->layout());
    if (cameraLayout) {
        // Check if a button with the same camera name already exists
        bool cameraExists = false;
        for (int i = 0; i < cameraLayout->count(); ++i) {
            QPushButton *button = qobject_cast<QPushButton*>(cameraLayout->itemAt(i)->widget());
            if (button && button->text() == camera.second) {
                // Button with the same camera name already exists, update its text and URL
                button->setText(camera.second);
                // Assuming the URL is stored as a tooltip for the button
                button->setToolTip(camera.first);
                cameraExists = true;
                break;
            }
        }

        if (!cameraExists) {
            // Add the camera button to the layout
            QPushButton *cameraButton = new QPushButton(camera.second);
            // Store the URL as a tooltip for the button
            cameraButton->setToolTip(camera.first);
            cameraButton->setIcon(QIcon("Icons/cctv.png"));
            cameraLayout->addWidget(cameraButton);

            // Connect the button's clicked signal to open the corresponding camera tab
            connect(cameraButton, &QPushButton::clicked, this, [=]()
                    {
                        if (cameraScreens) {
                            // Retrieve the URL from the button's tooltip
                            QString url = cameraButton->toolTip();
                            cameraScreens->addCamera(url, camera.second);
                        }
                    });
        }
    }
    else
    {
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
    }
    else {
        qDebug() << "Error: cameranames layout is not a QVBoxLayout";
    }
}

void MainWindow::on_one_layout_clicked()
{
    cameraScreens->on_one_camera_clicked();
}

void MainWindow::on_four_layout_clicked()
{
    cameraScreens->on_four_camera_clicked();
}

void MainWindow::on_sixteen_layout_clicked()
{
    cameraScreens->on_sixteen_camera_clicked();
}

