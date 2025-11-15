#include "root.h"
#include "ui_mainwindow.h"
#include "include/service/abstract/application_service.h"
#include <QCoreApplication>
#include <QDir>

MainWindow::MainWindow(std::shared_ptr<IApplicationService> application_service, QWidget *parent)
    : QMainWindow(parent),
    application_service_(std::move(application_service)),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);  // tu ładuje się mainwindow.ui

    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);

    while (!dir.exists("ludb") && dir.cdUp()) {
    }

    QString dataPath = dir.absoluteFilePath("ludb/90.dat");
    application_service_->Load(dataPath);

    // TODO(Oliwia): rewiry Oliwii Rewer xd - trzeba dodać przyciski, itp
    // TODO(Sonia): obsługa wykresów
}

MainWindow::~MainWindow() {
    delete ui;
}
