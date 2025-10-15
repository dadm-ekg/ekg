#include "root.h"

#include <QApplication>

#include "include/repository/signal_repository.h"
#include "include/service/application_service.h"
#include "src/repository/csv_signal_repository.cpp"
#include "src/service/application_service.cpp"

int main(int argc, char *argv[]) {
    std::shared_ptr<ISignalRepository> csvSignalRepository = std::make_shared<CSVSignalRepository>();
    std::shared_ptr<IApplicationService> applicationService = std::make_shared<ApplicationService>(csvSignalRepository);

    QApplication a(argc, argv);
    MainWindow w(applicationService);
    w.show();
    return a.exec();
}
