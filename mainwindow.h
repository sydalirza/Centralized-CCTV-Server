// mainwindow.h

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include<QGridLayout>
#include <QMainWindow>
#include <QLabel>  // Add this line

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

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
