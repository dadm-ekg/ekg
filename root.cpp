#include "root.h"
#include "include/service/application_service.h"

MainWindow::MainWindow(std::shared_ptr<IApplicationService> application_service, QWidget *parent)
    : QMainWindow(parent), application_service_(std::move(application_service)) {
    application_service_->Load("data.csv");

    // TODO(Oliwia Rewer): rewiry Oliwii Rewer
}

MainWindow::~MainWindow() = default;
