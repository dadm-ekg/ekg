#include "root.h"
#include "include/service/abstract/application_service.h"

MainWindow::MainWindow(std::shared_ptr<IApplicationService> application_service, QWidget *parent)
    : QMainWindow(parent), application_service_(std::move(application_service)) {
    application_service_->Load("../../../../ludb/90.dat");

    // TODO(Oliwia): rewiry Oliwii Rewer xd - trzeba dodać przyciski, itp
    // TODO(Sonia): obsługa wykresów
}

MainWindow::~MainWindow() = default;
