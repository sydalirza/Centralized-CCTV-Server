#ifndef FACESHANDLER_H
#define FACESHANDLER_H

#include <QWidget>
#include <QtSql/QSqlTableModel>
#include <QItemSelection>
#include "dlib_utils.h"
#include <opencv2/opencv.hpp>


namespace Ui {
class faceshandler;
}

class faceshandler : public QWidget
{
    Q_OBJECT

public:
    explicit faceshandler(QWidget *parent = nullptr);
    ~faceshandler();

    void fetch_faceEncodings();

signals:
    void add_face(dlib::matrix<float, 0, 1>& face_encoding);
    void delete_face(int num);

private slots:
    void on_load_image_button_clicked();

    void on_clear_button_clicked();

    void on_save_details_button_clicked();

    void on_update_button_clicked();

    void on_delete_button_clicked();

    void on_selection_changed(const QItemSelection &selected, const QItemSelection &deselected); // Add this line

private:
    Ui::faceshandler *ui;
    QSqlTableModel *model;
    QSqlDatabase db1;

    bool facedetected = false;

    dlib::matrix<dlib::rgb_pixel> face_chip;
    cv::Mat img = cv::imread("Icons/blank_new.jpeg");
    QImage blank_img = QImage(img.data, img.cols, img.rows, img.step, QImage::Format_RGB888);

    void change_state();
    void update_table();
    void update_faces();
};

#endif // FACESHANDLER_H
