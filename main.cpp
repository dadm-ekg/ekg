#include "root.h"

#include <QApplication>

#include "include/repository/signal_repository.h"
#include "include/service/application_service.h"
#include "include/service/filter_service.h"
#include "include/service/r_peaks_detection_service.h"
#include "include/service/hrv_geo_processing_service.h"
#include "include/service/hrv_time_processing_service.h"
#include "src/repository/csv_signal_repository.cpp"
#include "src/service/application_service.cpp"
#include "src/service/r_peaks_detection_service.cpp"
#include "src/service/moving_average_filter_service.cpp"
#include "src/service/butterworth_filter_service.cpp"
#include "src/service/hrv_time_processing_service.cpp"
#include "src/service/hrv_geo_processing_service.cpp"

int main(int argc, char *argv[]) {
    std::shared_ptr<ISignalRepository> signal_repository = std::make_shared<CSVSignalRepository>();

    std::shared_ptr<IFilterService> butterworth_filter_service = std::make_shared<ButterworthFilterService>();
    std::shared_ptr<IFilterService> moving_average_filter_service = std::make_shared<MovingAverageFilterService>();

    std::shared_ptr<IRPeaksDetectionService> r_peaks_detection_service = std::make_shared<RPeaksDetectionService>();

    std::shared_ptr<IHRVTimeProcessingService> hrv_time_processing_service = std::make_shared<
        HRVTimeProcessingService>();
    std::shared_ptr<IHRVGeoProcessingService> hrv_geo_processing_service = std::make_shared<HRVGeoProcessingService>();

    std::shared_ptr<IApplicationService> application_service = std::make_shared<ApplicationService>(
        signal_repository,
        butterworth_filter_service,
        moving_average_filter_service,
        r_peaks_detection_service,
        hrv_time_processing_service,
        hrv_geo_processing_service);

    QApplication a(argc, argv);
    MainWindow w(application_service);
    w.show();
    return a.exec();
}
