#include "../../include/bridge/analysis_worker.h"

AnalysisWorker::AnalysisWorker(std::shared_ptr<IApplicationService> application_service, QObject *parent)
    : QObject(parent), application_service_(std::move(application_service)) {
}

void AnalysisWorker::runBaseline(FilterMethod filterMethod) {
    QString filterName;
    bool success = false;
    QString errorMessage;

    switch (filterMethod) {
        case MovingAverage:
            filterName = "Moving Average";
            success = application_service_->RunFiltering(MovingAverage);
            break;
        case Butterworth:
            filterName = "Butterworth";
            success = application_service_->RunFiltering(Butterworth);
            break;
        case SavitzkyGolay:
            errorMessage = "Filtr Savitzky-Golay nie jest jeszcze zaimplementowany";
            success = false;
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

