#include "root.h"

#include <QApplication>

#include "include/repository/signal_repository.h"
#include "include/service/application_service.h"
#include "include/service/filter_service.h"
#include "include/service/r_peaks_detection_service.h"
#include "include/service/filter_service.h"
#include "src/repository/csv_signal_repository.cpp"
#include "src/service/application_service.cpp"
#include "src/service/r_peaks_detection_service.cpp"
#include "src/service/moving_average_filter_service.cpp"
#include "src/service/butterworth_filter_service.cpp"

int main(int argc, char *argv[]) {
    std::shared_ptr<ISignalRepository> csvSignalRepository = std::make_shared<CSVSignalRepository>();
    std::shared_ptr<IRPeaksDetectionService> rPeaksDetectionService = std::make_shared<RPeaksDetectionService>();
    std::shared_ptr<IFilterService> butterworthFilterService = std::make_shared<ButterworthFilterService>();
    std::shared_ptr<IFilterService> movingAverageFilterService = std::make_shared<MovingAverageFilterService>();
    std::shared_ptr<IApplicationService> applicationService = std::make_shared<ApplicationService>(
        csvSignalRepository, butterworthFilterService, movingAverageFilterService, rPeaksDetectionService);

    QApplication a(argc, argv);
    MainWindow w(applicationService);
    w.show();
    return a.exec();
}
