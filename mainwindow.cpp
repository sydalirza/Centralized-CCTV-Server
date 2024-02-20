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
    this->resize(screenSize.width() * 0.8, screenSize.height() * 0.8);
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

void MainWindow::on_close_button_clicked()
{
    qApp->quit();
}


void MainWindow::on_cameras_button_clicked()
{
    if(!tab_already_open("Cameras"))
    {
        ui -> tabWidget -> addTab(new CameraSettings(), "Cameras");
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

