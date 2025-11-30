#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <memory>

#include "include/bridge/ekg_controller.h"
#include "include/service/application_service.h"
#include "include/repository/dat_signal_repository.h"
#include "include/service/butterworth_filter_service.h"
#include "include/service/moving_average_filter_service.h"
#include "include/service/r_peaks_detection_service.h"
#include "include/service/hrv_time_processing_service.h"
#include "include/service/hrv_geo_processing_service.h"
#include "include/service/hrv_dfa_processing_service.h"
#include "include/service/waves_detection_service.h"
#include "include/service/heart_class_detection_service.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    auto signal_repository = std::make_shared<DATSignalRepository>();
    auto butterworth_filter_service = std::make_shared<ButterworthFilterService>();
    auto moving_average_filter_service = std::make_shared<MovingAverageFilterService>();
    auto r_peaks_detection_service = std::make_shared<RPeaksDetectionService>();
    auto hrv_time_processing_service = std::make_shared<HRVTimeProcessingService>();
    auto hrv_geo_processing_service = std::make_shared<HRVGeoProcessingService>();
    auto hrv_dfa_processing_service = std::make_shared<HRVDFAProcessingService>();
    auto waves_detection_service = std::make_shared<WavesDetectionService>();
    auto heart_class_detection_service = std::make_shared<HeartClassDetectionService>();

    auto application_service = std::make_shared<ApplicationService>(
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

    auto ekg_controller = new EkgController(application_service);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("ekgController", ekg_controller);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load(QUrl(QStringLiteral("qrc:/mainwindow.qml")));

    return app.exec();
}
