#ifndef ANALYSIS_WORKER_H
#define ANALYSIS_WORKER_H

#include <QObject>
#include <memory>
#include "../service/abstract/application_service.h"
#include "../dto/filter_method.h"
#include "../dto/r_peaks_detection_method.h"
#include "../dto/hrv_spectral_method.h"
#include "../dto/hrv_time_metrics.h"
#include "../dto/hrv_geo_metrics.h"
#include "../dto/hrv_dfa_metrics.h"
#include "../dto/heart_class_result.h"

class AnalysisWorker : public QObject {
    Q_OBJECT

public:
    explicit AnalysisWorker(std::shared_ptr<IApplicationService> application_service, QObject *parent = nullptr);

public slots:
    void loadFile(const QString &filename);
    void loadFileByName(const QString &filename);
    void runBaseline(FilterMethod filterMethod, int windowSize, int polynomialOrder);
    void runRPeaksDetection(RPeaksDetectionMethod method);
    void runHRVTime(HRVSpectralMethod method);
    void runHRVGeo();
    void runHRVDFA();
    void runWaves();
    void runHeartClass();

signals:
    void fileLoadCompleted(bool success, QString filename, QString errorMessage);
    void baselineCompleted(bool success, QString filterName, QString errorMessage);
    void rPeaksDetectionCompleted(bool success, QString methodName, QString errorMessage);
    void hrvTimeCompleted(bool success, QString methodName, HRVTimeMetrics metrics, QString errorMessage);
    void hrvGeoCompleted(bool success, HRVGeoMetrics metrics, QString errorMessage);
    void hrvDfaCompleted(bool success, HRVDFAMetrics metrics, QString errorMessage);
    void wavesCompleted(bool success, QString errorMessage);
    void heartClassCompleted(bool success, HeartClassResult result, QString errorMessage);

private:
    std::shared_ptr<IApplicationService> application_service_;
};

#endif

