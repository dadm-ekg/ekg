#include "root.h"
#include "ui_mainwindow.h"
#include "include/service/abstract/application_service.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(std::shared_ptr<IApplicationService> application_service, QWidget *parent)
    : QMainWindow(parent),
    application_service_(std::move(application_service)),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::onLoadDataClicked);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::onLoadDataClicked() {
    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);

    while (!dir.exists("ludb") && dir.cdUp()) {
    }

    QString ludbPath = dir.absoluteFilePath("ludb");

    QString filename = QFileDialog::getOpenFileName(
        this,
        "Wybierz plik danych EKG",
        ludbPath,
        "DAT Files (*.dat);;All Files (*)"
    );

    if (!filename.isEmpty()) {
        bool success = application_service_->Load(filename);
        
        if (success) {
            statusBar()->showMessage(QString("Załadowano plik: %1").arg(filename), 5000);
        } else {
            QMessageBox::warning(this, "Błąd", "Nie udało się załadować pliku.");
        }
    }
}
