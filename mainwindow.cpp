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
    int tabIndex = ui->tabWidget->addTab(defaultTab, "Default Tab");
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

void MainWindow::on_pushButton_4_clicked()
{
    qApp->quit();
}

