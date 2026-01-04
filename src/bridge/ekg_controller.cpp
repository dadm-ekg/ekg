#include "../../include/bridge/ekg_controller.h"
#include "../../include/dto/r_peaks_detection_method.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QPointF>
#include <QVariant>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

EkgController::EkgController(std::shared_ptr<IApplicationService> application_service, QObject *parent)
    : QObject(parent), application_service_(std::move(application_service)) {
}

void EkgController::loadData(const QString &filename) {
    if (filename.isEmpty()) {
        emit fileLoadError("Nie wybrano pliku");
        return;
    }

    bool success = application_service_->Load(filename);

    if (success) {
        baseline_completed_ = false;
        r_peaks_completed_ = false;
        hrv_time_completed_ = false;
        hrv_geo_completed_ = false;
        waves_completed_ = false;
        emit loadedFilenameChanged();
        emit isFileLoadedChanged();
        emit hasDataChanged();
        emit baselineCompletedChanged();
        emit rPeaksCompletedChanged();
        emit hrvTimeCompletedChanged();
        emit hrvGeoCompletedChanged();
        emit wavesCompletedChanged();
        emit fileLoadSuccess(filename);
    } else {
        emit fileLoadError("Nie udało się załadować pliku");
    }
}

void EkgController::openFileDialog() {
    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);

    while (!dir.exists("ludb") && dir.cdUp()) {
    }

    QString ludbPath = dir.absoluteFilePath("ludb");

    QString filename = QFileDialog::getOpenFileName(
        nullptr,
        "Wybierz plik danych EKG",
        ludbPath,
        "DAT Files (*.dat);;All Files (*)"
    );

    if (!filename.isEmpty()) {
        loadData(filename);
    }
}

bool EkgController::runBaseline(int filterMethod) {
    if (!hasData()) {
        emit filteringError("Nie załadowano danych. Najpierw zaimportuj sygnał.");
        return false;
    }

    QString filterName;
    bool success = false;

    switch (filterMethod) {
        case FilterMethod::MovingAverage:
            filterName = "Moving Average";
            success = application_service_->RunFiltering(::MovingAverage);
            break;
        case FilterMethod::Butterworth:
            filterName = "Butterworth";
            success = application_service_->RunFiltering(::Butterworth);
            break;
        case FilterMethod::SavitzkyGolay:
            emit filteringError("Filtr Savitzky-Golay nie jest jeszcze zaimplementowany");
            return false;
        default:
            emit filteringError("Nieznany typ filtra");
            return false;
    }

    if (success) {
        baseline_completed_ = true;
        r_peaks_completed_ = false;
        hrv_time_completed_ = false;
        hrv_geo_completed_ = false;
        waves_completed_ = false;
        emit hasFilteredDataChanged();
        emit baselineCompletedChanged();
        emit rPeaksCompletedChanged();
        emit hrvTimeCompletedChanged();
        emit hrvGeoCompletedChanged();
        emit wavesCompletedChanged();
        emit filteringSuccess(filterName);
    } else {
        emit filteringError("Nie udało się zastosować filtra " + filterName);
    }

    return success;
}

bool EkgController::runRPeaksDetection(int method) {
    if (!hasFilteredData()) {
        emit rPeaksDetectionError("Brak przefiltrowanych danych. Najpierw uruchom filtrowanie baseline.");
        return false;
    }

    QString methodName;
    RPeaksDetectionMethod rPeaksMethod;

    switch (method) {
        case RPeaksMethod::PanTompkins:
            methodName = "Pan-Tompkins";
            rPeaksMethod = RPeaksDetectionMethod::PanTompkins;
            break;
        case RPeaksMethod::Hilbert:
            methodName = "Transformata Hilberta";
            rPeaksMethod = RPeaksDetectionMethod::Hilbert;
            break;
        case RPeaksMethod::Wavelet:
            methodName = "Falkowa (Wavelet)";
            rPeaksMethod = RPeaksDetectionMethod::Wavelet;
            break;
        default:
            emit rPeaksDetectionError("Nieznana metoda detekcji");
            return false;
    }

    bool success = application_service_->CalculateRPeaks(rPeaksMethod);

    if (success) {
        r_peaks_completed_ = true;
        hrv_time_completed_ = false;
        hrv_geo_completed_ = false;
        waves_completed_ = false;
        emit rPeaksCompletedChanged();
        emit hrvTimeCompletedChanged();
        emit hrvGeoCompletedChanged();
        emit wavesCompletedChanged();
        emit rPeaksDetectionSuccess(methodName);
    } else {
        emit rPeaksDetectionError("Nie udało się wykryć pików R metodą " + methodName);
    }

    return success;
}

bool EkgController::runHRVTime(int method) {
    if (!rPeaksCompleted()) {
        emit hrvTimeError("Brak wykrytych pików R. Najpierw uruchom detekcję pików R.");
        return false;
    }

    QString methodName;
    HRVTimeMetrics::SpectralMethod spectralMethod;

    switch (method) {
        case HRVSpectralMethod::ClassicPeriodogram:
            methodName = "Klasyczny periodogram";
            spectralMethod = HRVTimeMetrics::SpectralMethod::CLASSIC_PERIODOGRAM;
            break;
        case HRVSpectralMethod::LombScargle:
            methodName = "Lomb-Scargle";
            spectralMethod = HRVTimeMetrics::SpectralMethod::LOMB_SCARGLE;
            break;
        case HRVSpectralMethod::Welch:
            methodName = "Welch";
            spectralMethod = HRVTimeMetrics::SpectralMethod::WELCH;
            break;
        default:
            emit hrvTimeError("Nieznana metoda estymacji widma");
            return false;
    }

    cached_hrv_metrics_ = application_service_->CalculateHRVTime(spectralMethod);
    hrv_time_completed_ = true;
    emit hrvTimeCompletedChanged();
    emit hrvTimeSuccess(methodName);

    return true;
}

bool EkgController::runHRVGeo() {
    if (!rPeaksCompleted()) {
        emit hrvGeoError("Brak wykrytych pików R. Najpierw uruchom detekcję pików R.");
        return false;
    }

    cached_hrv_geo_metrics_ = application_service_->CalculateHRVGeo();
    hrv_geo_completed_ = true;
    emit hrvGeoCompletedChanged();
    emit hrvGeoSuccess();

    return true;
}

bool EkgController::runWaves() {
    if (!hasFilteredData()) {
        emit wavesError("Brak przefiltrowanych danych. Najpierw uruchom filtrowanie baseline.");
        return false;
    }

    bool success = application_service_->CalculateWaves();

    if (success) {
        waves_completed_ = true;
        emit wavesCompletedChanged();
        emit wavesSuccess();
    } else {
        emit wavesError("Nie udało się wykryć fal EKG");
    }

    return success;
}

QString EkgController::loadedFilename() const {
    return application_service_->GetLoadedFilename();
}

bool EkgController::isFileLoaded() const {
    return application_service_->IsFileLoaded();
}

bool EkgController::hasData() const {
    return application_service_->GetData() != nullptr;
}

bool EkgController::hasFilteredData() const {
    return application_service_->GetFilteredData() != nullptr;
}

bool EkgController::baselineCompleted() const {
    return baseline_completed_;
}

bool EkgController::rPeaksCompleted() const {
    return r_peaks_completed_;
}

bool EkgController::hrvTimeCompleted() const {
    return hrv_time_completed_;
}

bool EkgController::hrvGeoCompleted() const {
    return hrv_geo_completed_;
}

bool EkgController::wavesCompleted() const {
    return waves_completed_;
}

void EkgController::resetHRVTime() {
    hrv_time_completed_ = false;
    cached_hrv_metrics_ = HRVTimeMetrics{};
    emit hrvTimeCompletedChanged();
}

void EkgController::resetHRVGeo() {
    hrv_geo_completed_ = false;
    cached_hrv_geo_metrics_ = HRVGeoMetrics{};
    emit hrvGeoCompletedChanged();
}

void EkgController::resetWaves() {
    waves_completed_ = false;
    emit wavesCompletedChanged();
}

QStringList EkgController::getAvailableFiles() const {
    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);

    while (!dir.exists("ludb") && dir.cdUp()) {
    }

    QString ludbPath = dir.absoluteFilePath("ludb");
    QDir ludbDir(ludbPath);

    if (!ludbDir.exists()) {
        return QStringList();
    }

    QStringList filters;
    filters << "*.dat";
    ludbDir.setNameFilters(filters);
    ludbDir.setSorting(QDir::Name);

    QStringList files = ludbDir.entryList(QDir::Files);
    
    QStringList fileBasenames;
    for (const QString &file : files) {
        QFileInfo fileInfo(file);
        fileBasenames.append(fileInfo.completeBaseName());
    }

    return fileBasenames;
}

void EkgController::loadFileByName(const QString &filename) {
    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);

    while (!dir.exists("ludb") && dir.cdUp()) {
    }

    QString ludbPath = dir.absoluteFilePath("ludb");
    QString fullPath = ludbPath + "/" + filename + ".dat";

    loadData(fullPath);
}

void EkgController::resetBaseline() {
    application_service_->ClearFilteredData();
    baseline_completed_ = false;
    r_peaks_completed_ = false;
    hrv_time_completed_ = false;
    hrv_geo_completed_ = false;
    waves_completed_ = false;
    emit hasFilteredDataChanged();
    emit baselineCompletedChanged();
    emit rPeaksCompletedChanged();
    emit hrvTimeCompletedChanged();
    emit hrvGeoCompletedChanged();
    emit wavesCompletedChanged();
}

void EkgController::resetRPeaks() {
    application_service_->ClearRPeaks();
    r_peaks_completed_ = false;
    hrv_time_completed_ = false;
    hrv_geo_completed_ = false;
    waves_completed_ = false;
    emit rPeaksCompletedChanged();
    emit hrvTimeCompletedChanged();
    emit hrvGeoCompletedChanged();
    emit wavesCompletedChanged();
}

namespace {
QVariantList buildSeries(const std::shared_ptr<SignalDataset> &dataset, int channel, int maxPoints) {
    QVariantList series;
    if (!dataset || dataset->values.empty()) return series;

    const int channelCount = static_cast<int>(dataset->values.front().channelValues.size());
    if (channelCount == 0) return series;

    const int clampedChannel = std::max(0, std::min(channel, channelCount - 1));
    const double frequency = dataset->frequency > 0 ? static_cast<double>(dataset->frequency) : 1.0;

    const int sampleCount = static_cast<int>(dataset->values.size());
    const int stride = std::max(1, static_cast<int>(std::ceil(static_cast<double>(sampleCount) / std::max(1, maxPoints))));

    series.reserve(sampleCount / stride + 1);

    for (int i = 0; i < sampleCount; i += stride) {
        const auto &point = dataset->values[static_cast<size_t>(i)];
        if (static_cast<size_t>(clampedChannel) >= point.channelValues.size()) continue;
        const double t = static_cast<double>(i) / frequency;
        QVariantMap entry;
        entry["x"] = t;
        entry["y"] = static_cast<double>(point.channelValues[static_cast<size_t>(clampedChannel)]);
        series.append(entry);
    }

    return series;
}
}

QVariantList EkgController::getRawSeries(int channel, int maxPoints) const {
    return buildSeries(application_service_->GetData(), channel, maxPoints);
}

QVariantList EkgController::getFilteredSeries(int channel, int maxPoints) const {
    return buildSeries(application_service_->GetFilteredData(), channel, maxPoints);
}

QVariantList EkgController::getRPeakMarkers(int channel) const {
    QVariantList markers;

    const auto peaks = application_service_->GetRPeaks();
    if (!peaks || peaks->empty()) return markers;

    const auto filtered = application_service_->GetFilteredData();
    if (!filtered || filtered->values.empty()) return markers;

    const int channelCount = static_cast<int>(filtered->values.front().channelValues.size());
    if (channelCount == 0) return markers;

    const int clampedChannel = std::max(0, std::min(channel, channelCount - 1));
    const double frequency = filtered->frequency > 0 ? static_cast<double>(filtered->frequency) : 1.0;

    markers.reserve(peaks->size());

    for (size_t i = 0; i < peaks->size(); ++i) {
        if (!(*peaks)[i].peak) continue;
        const auto &point = filtered->values[i];
        if (static_cast<size_t>(clampedChannel) >= point.channelValues.size()) continue;
        const double t = static_cast<double>(i) / frequency;
        QVariantMap entry;
        entry["x"] = t;
        entry["y"] = static_cast<double>(point.channelValues[static_cast<size_t>(clampedChannel)]);
        markers.append(entry);
    }

    return markers;
}

QVariantMap EkgController::getHRVTimeMetrics() const {
    QVariantMap metrics;
    metrics["rr_mean"] = static_cast<double>(cached_hrv_metrics_.rr_mean);
    metrics["sdnn"] = static_cast<double>(cached_hrv_metrics_.sdnn);
    metrics["rmssd"] = static_cast<double>(cached_hrv_metrics_.rmssd);
    metrics["tp"] = static_cast<double>(cached_hrv_metrics_.tp);
    metrics["vlf"] = static_cast<double>(cached_hrv_metrics_.vlf);
    metrics["lf"] = static_cast<double>(cached_hrv_metrics_.lf);
    metrics["hf"] = static_cast<double>(cached_hrv_metrics_.hf);
    metrics["lf_hf"] = static_cast<double>(cached_hrv_metrics_.lf_hf);
    return metrics;
}

QVariantMap EkgController::getHRVGeoMetrics() const {
    QVariantMap metrics;
    metrics["triangular_index"] = cached_hrv_geo_metrics_.triangular_index;
    metrics["tinn"] = cached_hrv_geo_metrics_.tinn;
    metrics["sd1"] = cached_hrv_geo_metrics_.sd1;
    metrics["sd2"] = cached_hrv_geo_metrics_.sd2;
    return metrics;
}

QVariantList EkgController::getHRVGeoHistogram() const {
    QVariantList result;
    const auto& hist = cached_hrv_geo_metrics_.histogram;
    double binWidth = cached_hrv_geo_metrics_.bin_width;
    double rrMin = cached_hrv_geo_metrics_.rr_min;

    for (size_t i = 0; i < hist.size(); ++i) {
        QVariantMap entry;
        entry["x"] = rrMin + i * binWidth + binWidth / 2.0;
        entry["y"] = hist[i];
        result.append(entry);
    }
    return result;
}

QVariantList EkgController::getHRVGeoPoincare() const {
    QVariantList result;
    const auto& rr = cached_hrv_geo_metrics_.rr_intervals;

    for (size_t i = 0; i + 1 < rr.size(); ++i) {
        QVariantMap entry;
        entry["x"] = rr[i];
        entry["y"] = rr[i + 1];
        result.append(entry);
    }
    return result;
}

QVariantMap EkgController::getWaveMarkers(int channel) const {
    QVariantMap result;
    QVariantList pOnsets, pEnds, qrsOnsets, qrsEnds, tEnds;

    const auto waves = application_service_->GetWaves();
    if (!waves || waves->empty()) return result;

    const auto filtered = application_service_->GetFilteredData();
    if (!filtered || filtered->values.empty()) return result;

    const int channelCount = static_cast<int>(filtered->values.front().channelValues.size());
    if (channelCount == 0) return result;

    const int clampedChannel = std::max(0, std::min(channel, channelCount - 1));
    const double frequency = filtered->frequency > 0 ? static_cast<double>(filtered->frequency) : 1.0;

    for (size_t i = 0; i < waves->size(); ++i) {
        const auto& wave = (*waves)[i];
        if (static_cast<size_t>(clampedChannel) >= wave.channelValues.size()) continue;
        
        const double t = static_cast<double>(i) / frequency;
        const double y = static_cast<double>(wave.channelValues[static_cast<size_t>(clampedChannel)]);

        if (wave.p_wave_onset) {
            QVariantMap entry;
            entry["x"] = t;
            entry["y"] = y;
            pOnsets.append(entry);
        }
        if (wave.p_wave_end) {
            QVariantMap entry;
            entry["x"] = t;
            entry["y"] = y;
            pEnds.append(entry);
        }
        if (wave.qrs_onset) {
            QVariantMap entry;
            entry["x"] = t;
            entry["y"] = y;
            qrsOnsets.append(entry);
        }
        if (wave.qrs_end) {
            QVariantMap entry;
            entry["x"] = t;
            entry["y"] = y;
            qrsEnds.append(entry);
        }
        if (wave.t_end) {
            QVariantMap entry;
            entry["x"] = t;
            entry["y"] = y;
            tEnds.append(entry);
        }
    }

    result["p_onsets"] = pOnsets;
    result["p_ends"] = pEnds;
    result["qrs_onsets"] = qrsOnsets;
    result["qrs_ends"] = qrsEnds;
    result["t_ends"] = tEnds;

    return result;
}

int EkgController::channelCount() const {
    auto data = application_service_->GetData();
    if (data && !data->values.empty()) {
        return static_cast<int>(data->values.front().channelValues.size());
    }

    auto filtered = application_service_->GetFilteredData();
    if (filtered && !filtered->values.empty()) {
        return static_cast<int>(filtered->values.front().channelValues.size());
    }

    return 0;
}

double EkgController::samplingFrequency() const {
    auto data = application_service_->GetData();
    if (data && data->frequency > 0) return static_cast<double>(data->frequency);

    auto filtered = application_service_->GetFilteredData();
    if (filtered && filtered->frequency > 0) return static_cast<double>(filtered->frequency);

    return 0.0;
}
