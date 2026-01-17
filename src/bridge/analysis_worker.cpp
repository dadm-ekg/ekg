#include "../../include/bridge/analysis_worker.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

AnalysisWorker::AnalysisWorker(std::shared_ptr<IApplicationService> application_service, QObject *parent)
    : QObject(parent), application_service_(std::move(application_service)) {
}

void AnalysisWorker::loadFile(const QString &filename) {
    if (filename.isEmpty()) {
        emit fileLoadCompleted(false, "", "Nie wybrano pliku");
        return;
    }

    bool success = application_service_->Load(filename);
    QString errorMessage;

    if (success) {
        emit fileLoadCompleted(true, filename, "");
    } else {
        QString lastError = application_service_->GetLastValidationError();
        if (lastError.isEmpty()) {
            errorMessage = "Nie udało się załadować pliku";
        } else {
            errorMessage = lastError;
        }
        emit fileLoadCompleted(false, filename, errorMessage);
    }
}

void AnalysisWorker::loadFileByName(const QString &filename) {
    if (filename.isEmpty()) {
        emit fileLoadCompleted(false, "", "Nie wybrano pliku");
        return;
    }

    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);

    while (!dir.exists("ludb") && dir.cdUp()) {
    }

    QString fullPath;
    
    if (filename.startsWith("LUDB/")) {
        QString baseName = filename.mid(5);
        QString ludbPath = dir.absoluteFilePath("ludb");
        fullPath = ludbPath + "/" + baseName + ".dat";
    } else if (filename.startsWith("MITBIH/")) {
        QString baseName = filename.mid(7);
        QString mitbihPath = dir.absoluteFilePath("mitbih");
        fullPath = mitbihPath + "/" + baseName + ".dat";
    } else {
        QString ludbPath = dir.absoluteFilePath("ludb");
        fullPath = ludbPath + "/" + filename + ".dat";
    }

    loadFile(fullPath);
}

void AnalysisWorker::runBaseline(FilterMethod filterMethod, int windowSize, int polynomialOrder) {
    QString filterName;
    bool success = false;
    QString errorMessage;

    switch (filterMethod) {
        case MovingAverage:
            filterName = "Moving Average";
            success = application_service_->RunFiltering(MovingAverage, windowSize, polynomialOrder);
            break;
        case Butterworth:
            filterName = "Butterworth";
            success = application_service_->RunFiltering(Butterworth, windowSize, polynomialOrder);
            break;
        case SavitzkyGolay:
            filterName = "Savitzky-Golay";
            success = application_service_->RunFiltering(SavitzkyGolay, windowSize, polynomialOrder);
            break;
        default:
            errorMessage = "Nieznany typ filtra";
            success = false;
            break;
    }

    if (!success && errorMessage.isEmpty()) {
        errorMessage = "Nie udało się zastosować filtra " + filterName;
    }

    emit baselineCompleted(success, filterName, errorMessage);
}

void AnalysisWorker::runRPeaksDetection(RPeaksDetectionMethod method) {
    QString methodName;
    bool success = false;
    QString errorMessage;

    switch (method) {
        case PanTompkins:
            methodName = "Pan-Tompkins";
            success = application_service_->CalculateRPeaks(PanTompkins);
            break;
        case Hilbert:
            methodName = "Transformata Hilberta";
            success = application_service_->CalculateRPeaks(Hilbert);
            break;
        case Wavelet:
            methodName = "Falkowa (Wavelet)";
            success = application_service_->CalculateRPeaks(Wavelet);
            break;
        default:
            errorMessage = "Nieznana metoda detekcji";
            success = false;
            break;
    }

    if (!success && errorMessage.isEmpty()) {
        errorMessage = "Nie udało się wykryć pików R metodą " + methodName;
    }

    emit rPeaksDetectionCompleted(success, methodName, errorMessage);
}

void AnalysisWorker::runHRVTime(HRVSpectralMethod method) {
    QString methodName;
    HRVTimeMetrics::SpectralMethod spectralMethod;
    bool success = true;
    QString errorMessage;

    switch (method) {
        case ClassicPeriodogram:
            methodName = "Klasyczny periodogram";
            spectralMethod = HRVTimeMetrics::SpectralMethod::CLASSIC_PERIODOGRAM;
            break;
        case LombScargle:
            methodName = "Lomb-Scargle";
            spectralMethod = HRVTimeMetrics::SpectralMethod::LOMB_SCARGLE;
            break;
        case Welch:
            methodName = "Welch";
            spectralMethod = HRVTimeMetrics::SpectralMethod::WELCH;
            break;
        default:
            errorMessage = "Nieznana metoda estymacji widma";
            success = false;
            emit hrvTimeCompleted(false, "", HRVTimeMetrics{}, errorMessage);
            return;
    }

    HRVTimeMetrics metrics = application_service_->CalculateHRVTime(spectralMethod);
    emit hrvTimeCompleted(success, methodName, metrics, errorMessage);
}

void AnalysisWorker::runHRVGeo() {
    HRVGeoMetrics metrics = application_service_->CalculateHRVGeo();
    emit hrvGeoCompleted(true, metrics, "");
}

void AnalysisWorker::runWaves() {
    bool success = application_service_->CalculateWaves();
    QString errorMessage = success ? "" : "Nie udało się wykryć fal EKG";
    emit wavesCompleted(success, errorMessage);
}

void AnalysisWorker::runHeartClass() {
    HeartClassResult result = application_service_->CalculateHeartClass();
    emit heartClassCompleted(true, result, "");
}

