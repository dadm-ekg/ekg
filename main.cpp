#include "root.h"

#include <QApplication>

#include "include/repository/abstract/signal_repository.h"
#include "include/repository/dat_signal_repository.h"
#include "include/service/abstract/application_service.h"
#include "include/service/abstract/filter_service.h"
#include "include/service/abstract/r_peaks_detection_service.h"
#include "include/service/abstract/hrv_geo_processing_service.h"
#include "include/service/abstract/hrv_time_processing_service.h"
#include "include/service/application_service.h"
#include "include/service/butterworth_filter_service.h"
#include "include/service/moving_average_filter_service.h"
#include "include/service/r_peaks_detection_service.h"
#include "include/service/hrv_time_processing_service.h"
#include "include/service/hrv_geo_processing_service.h"
#include "include/service/hrv_dfa_processing_service.h"
#include "include/service/heart_class_detection_service.h"
#include "include/service/waves_detection_service.h"

int main(int argc, char *argv[]) {
    std::shared_ptr<ISignalRepository> signal_repository = std::make_shared<DATSignalRepository>();

    std::shared_ptr<IFilterService> butterworth_filter_service = std::make_shared<ButterworthFilterService>();
    std::shared_ptr<IFilterService> moving_average_filter_service = std::make_shared<MovingAverageFilterService>();

    std::shared_ptr<IRPeaksDetectionService> r_peaks_detection_service = std::make_shared<RPeaksDetectionService>();

    std::shared_ptr<IHRVTimeProcessingService> hrv_time_processing_service = std::make_shared<
        HRVTimeProcessingService>();
    std::shared_ptr<IHRVGeoProcessingService> hrv_geo_processing_service = std::make_shared<HRVGeoProcessingService>();

    std::shared_ptr<IWavesDetectionService> waves_detection_service = std::make_shared<WavesDetectionService>();
    std::shared_ptr<IHRVDFAProcessingService> hrv_dfa_processing_service = std::make_shared<HRVDFAProcessingService>();
    std::shared_ptr<IHeartClassDetectionService> heart_class_detection_service = std::make_shared<
        HeartClassDetectionService>();

    std::shared_ptr<IApplicationService> application_service = std::make_shared<ApplicationService>(
        signal_repository,
        butterworth_filter_service,
        moving_average_filter_service,
        r_peaks_detection_service,
        hrv_time_processing_service,
        hrv_geo_processing_service,
        hrv_dfa_processing_service,
        waves_detection_service,
        heart_class_detection_service
    );

    QApplication a(argc, argv);
    MainWindow w(application_service);
    w.show();
    return a.exec();
}
