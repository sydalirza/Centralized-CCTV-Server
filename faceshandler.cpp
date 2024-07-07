#include "faceshandler.h"
#include "dlib_utils.h"
#include "opencv2/imgcodecs.hpp"
#include "ui_faceshandler.h"
#include <QFile>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QFileDialog>
#include <opencv2/opencv.hpp>

faceshandler::faceshandler(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::faceshandler)
{
    ui->setupUi(this);

    // Connect to the SQLite database
    db1 = QSqlDatabase::addDatabase("QSQLITE", "connection2");
    db1.setDatabaseName("faces.db"); // Assuming the SQLite database file is named faces.db
    db1.open();


    bool dbExists = QFile::exists("faces.db");

    if (!db1.open()) {
        qDebug() << "Error: Failed to open database";
        QMessageBox::critical(this, "Database Error", "Failed to open the database.");
        return;
    }

    // If the database does not exist, create it
    if (!dbExists) {
        QSqlQuery query;
        QString createTableQuery = R"(
            CREATE TABLE IF NOT EXISTS face_encodings (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                age INTEGER,
                gender TEXT,
                encoding BLOB NOT NULL
                image_path TEXT NOT NULL
            )
        )";

        if (!query.exec(createTableQuery)) {
            qDebug() << "Error: Failed to create table 'face_encodings'" << query.lastError().text();
            QMessageBox::critical(this, "Database Error", "Failed to create table 'face_encodings'.");
            return;
        }
    }

    // Set up the model
    model = new QSqlTableModel(this, db1);
    update_table();

    initialize_network();
    initialize_shape_predictor();
    change_state();

    // Connect the selection signal to the slot
    connect(ui->registered_faces_tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &faceshandler::on_selection_changed);
}

faceshandler::~faceshandler()
{
    delete ui;
}

void faceshandler::update_table()
{
    model->setTable("face_encodings");

    if (!model->select()) {
        qDebug() << "Error: Failed to select data from table 'face_encodings'" << model->lastError().text();
        QMessageBox::critical(this, "Database Error", "Failed to retrieve data from table 'face_encodings'.");
        return;
    }

    model->setHeaderData(0, Qt::Horizontal, "ID");
    model->setHeaderData(1, Qt::Horizontal, "Name");
    model->setHeaderData(2, Qt::Horizontal, "Age");
    model->setHeaderData(3, Qt::Horizontal, "Gender");
    model->setHeaderData(4, Qt::Horizontal, "Encoding");
    model->setHeaderData(5, Qt::Horizontal, "Image Path");


    // Set the model to the QTableView
    ui->registered_faces_tableView->setModel(model);

    // Populate the model with data from the database
    model->select();

    // Resize columns to contents
    ui->registered_faces_tableView->resizeColumnsToContents();

    // Set stretch factors to make columns fill the entire table view
    for (int i = 0; i < model->columnCount(); ++i) {
        ui->registered_faces_tableView->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    }

}

void faceshandler::on_load_image_button_clicked()
{
    if (!facedetected)
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open Image"), "", tr("Image Files (*.png *.jpg *.bmp)"));
        if (fileName.isEmpty())
            return;

        ui->image_path->setText(fileName);

        cv::Mat img = cv::imread(fileName.toStdString());
        if (img.empty()) {
            QMessageBox::critical(this, "Image Error", "Failed to load the image.");
            return;
        }

        // Convert the image to dlib's format
        dlib::cv_image<dlib::bgr_pixel> dlib_img(img);

        // Detect faces
        dlib::frontal_face_detector detector = dlib::get_frontal_face_detector();
        std::vector<dlib::rectangle> faces = detector(dlib_img);

        if (faces.empty()) {
            QMessageBox::critical(this, "Face Detection Error", "No face detected in the image.");
            return;
        }
        else if (faces.size() > 1)
        {
            QMessageBox::critical(this, "Face Detection Error", "Expected exactly one face in " + fileName + ", but found " + QString::number(faces.size()) + " faces.");
            return;
        }

        // Assuming we only want the first detected face
        auto shape = sp(dlib_img, faces[0]);

        extract_image_chip(dlib_img, get_face_chip_details(shape, 150, 0.25), face_chip);

        // Convert dlib matrix to QImage
        QImage qimg(face_chip.nc(), face_chip.nr(), QImage::Format_RGB888);
        for (int y = 0; y < face_chip.nr(); ++y)
            for (int x = 0; x < face_chip.nc(); ++x)
                qimg.setPixel(x, y, qRgb(face_chip(y, x).red, face_chip(y, x).green, face_chip(y, x).blue));

        // Display the face in the QLabel
        ui->image_label->setPixmap(QPixmap::fromImage(qimg));
        facedetected = true;
        change_state();
    }
    else
    {
        QMessageBox::critical(this, "Error", "Please clear the previous instance first");
        return;
    }
}

void faceshandler::change_state()
{
    if (facedetected)
    {
        ui->load_image_button->setEnabled(false);
        ui->name->setEnabled(true);
        ui->age->setEnabled(true);
        ui->male_radio_button->setEnabled(true);
        ui->female_radio_button->setEnabled(true);
        ui->save_details_button->setEnabled(true);
        ui->clear_button->setEnabled(true);
    }
    else
    {
        ui->load_image_button->setEnabled(true);
        ui->name->setEnabled(false);
        ui->age->setEnabled(false);
        ui->male_radio_button->setEnabled(false);
        ui->female_radio_button->setEnabled(false);
        ui->save_details_button->setEnabled(false);
        ui->clear_button->setEnabled(false);
        ui->image_label->setPixmap(QPixmap::fromImage(blank_img));
        ui->image_path->clear();
    }
}



void faceshandler::on_clear_button_clicked()
{
    ui->name->clear();
    ui->age->clear();
    facedetected = false;
    change_state();
}


void faceshandler::on_save_details_button_clicked()
{
    if (facedetected)
    {
        // Validate inputs
        QString name = ui->name->text().trimmed();
        QString ageStr = ui->age->text().trimmed();
        bool isMale = ui->male_radio_button->isChecked();
        bool isFemale = ui->female_radio_button->isChecked();
        QString image_path = ui->image_path->text().trimmed();

        // Check if name is empty
        if (name.isEmpty())
        {
            QMessageBox::critical(this, "Input Error", "Please enter a name.");
            return;
        }

        // Check if age is an integer
        bool ok;
        int age = ageStr.toInt(&ok);
        if (!ok)
        {
            QMessageBox::critical(this, "Input Error", "Age must be a valid integer.");
            return;
        }

        // Check if gender is selected
        if (!isMale && !isFemale)
        {
            QMessageBox::critical(this, "Input Error", "Please select a gender.");
            return;
        }

        // Convert gender to string
        QString gender = isMale ? "Male" : "Female";

        dlib::matrix<float, 0, 1> face_encoding = net(face_chip);

        // Serialize the face encoding into a QByteArray
        QByteArray encodingBytes;
        QDataStream stream(&encodingBytes, QIODevice::WriteOnly);
        stream.writeRawData(reinterpret_cast<const char*>(face_encoding.begin()), face_encoding.size()*sizeof(float));

        // Save details to database
        QSqlQuery query(db1);
        query.prepare("INSERT INTO face_encodings (name, age, gender, encoding, image_path) VALUES (:name, :age, :gender, :encoding, :path)");
        query.bindValue(":name", name);
        query.bindValue(":age", age);
        query.bindValue(":gender", gender);
        query.bindValue(":encoding", encodingBytes);
        query.bindValue(":path", image_path);

        if (!query.exec()) {
            qDebug() << "Error inserting data into table 'face_encodings':" << query.lastError().text();
            QMessageBox::critical(this, "Database Error", "Failed to save details to the database.");
            return;
        }

        emit add_face(face_encoding);
        update_table();
        QMessageBox::information(this, "Success", "Details saved successfully.");

    }
}

void faceshandler::on_update_button_clicked()
{
    update_table();
}

void faceshandler::fetch_faceEncodings()
{
    QSqlQuery query(db1);
    query.prepare("SELECT encoding, image_path FROM face_encodings");

    if (!query.exec()) {
        qDebug() << "Error fetching data from table 'face_encodings':" << query.lastError().text();
        return;
    }

    while (query.next())
    {
        QByteArray encodingBytes = query.value("encoding").toByteArray();

        // Check the size of encodingBytes
        if (encodingBytes.size() % sizeof(float) != 0)
        {
            qDebug() << "Error: Encoding size mismatch.";
            continue; // Skip this entry if size is incorrect
        }

        // Initialize face_encoding dynamically
        dlib::matrix<float, 0, 1> face_encoding_enc;

        // Resize the matrix to accommodate data
        face_encoding_enc.set_size(encodingBytes.size() / sizeof(float), 1);

        // Copy data from QByteArray to dlib::matrix
        std::memcpy(face_encoding_enc.begin(), encodingBytes.constData(), encodingBytes.size());

        emit add_face(face_encoding_enc);
    }
}

void faceshandler::on_selection_changed(const QItemSelection &selected, const QItemSelection &deselected)
{
    // Check if any row is selected
    if (!ui->registered_faces_tableView->selectionModel()->selectedRows().isEmpty()) {
        ui->delete_button->setEnabled(true);
    } else {
        ui->delete_button->setEnabled(false);
    }
}

void faceshandler::on_delete_button_clicked()
{
    QModelIndexList selectedRows = ui->registered_faces_tableView->selectionModel()->selectedRows();

    if (selectedRows.isEmpty()) {
        QMessageBox::warning(this, "Selection Error", "No row selected.");
        return;
    }

    int selectedRow = selectedRows.first().row(); // Get the position (index) of the selected row
    qDebug() << selectedRow;

    // Confirm deletion
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Delete", "Are you sure you want to delete this entry?",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        // Get the ID of the selected row (assuming ID is in the first column)
        int id = model->data(model->index(selectedRow, 0)).toInt();

        // Delete the row from the database
        QSqlQuery query(db1);
        query.prepare("DELETE FROM face_encodings WHERE id = :id");
        query.bindValue(":id", id);

        if (!query.exec()) {
            qDebug() << "Error deleting data from table 'face_encodings':" << query.lastError().text();
            QMessageBox::critical(this, "Database Error", "Failed to delete the entry from the database.");
            return;
        }

        emit delete_face(selectedRow);
        update_table();
        QMessageBox::information(this, "Success", "Entry deleted successfully.");
    }
}
