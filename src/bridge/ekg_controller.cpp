#include "../../include/bridge/ekg_controller.h"
#include "../../include/bridge/analysis_worker.h"
#include "../../include/dto/r_peaks_detection_method.h"
#include "../../include/dto/file_format.h"
#include "../../include/repository/results_repository.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QPointF>
#include <QVariant>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>
#include <sstream>
#include <iomanip>

EkgController::EkgController(std::shared_ptr<IApplicationService> application_service, std::shared_ptr<IResultsRepository> results_repository, QObject *parent)
    : QObject(parent), application_service_(std::move(application_service)), results_repository_(std::move(results_repository)) {
    setupAnalysisWorker();
}

void EkgController::setupAnalysisWorker() {
    analysis_thread_ = new QThread(this);
    analysis_worker_ = new AnalysisWorker(application_service_);
    analysis_worker_->moveToThread(analysis_thread_);
    
    connect(analysis_thread_, &QThread::finished, analysis_worker_, &QObject::deleteLater);
    connect(analysis_worker_, &AnalysisWorker::baselineCompleted, this, [this](bool success, QString filterName, QString errorMessage) {
        if (success) {
            baseline_completed_ = true;
            r_peaks_completed_ = false;
            hrv_time_completed_ = false;
            hrv_geo_completed_ = false;
            waves_completed_ = false;
            heart_class_completed_ = false;
            emit hasFilteredDataChanged();
            emit baselineCompletedChanged();
            emit rPeaksCompletedChanged();
            emit hrvTimeCompletedChanged();
            emit hrvGeoCompletedChanged();
            emit wavesCompletedChanged();
            emit heartClassCompletedChanged();
            emit filteringSuccess(filterName);
        } else {
            emit filteringError(errorMessage);
        }
    });
    
    connect(analysis_worker_, &AnalysisWorker::rPeaksDetectionCompleted, this, [this](bool success, QString methodName, QString errorMessage) {
        if (success) {
            r_peaks_completed_ = true;
            hrv_time_completed_ = false;
            hrv_geo_completed_ = false;
            waves_completed_ = false;
            heart_class_completed_ = false;
            emit rPeaksCompletedChanged();
            emit hrvTimeCompletedChanged();
            emit hrvGeoCompletedChanged();
            emit wavesCompletedChanged();
            emit heartClassCompletedChanged();
            emit rPeaksDetectionSuccess(methodName);
        } else {
            emit rPeaksDetectionError(errorMessage);
        }
    });
    
    connect(analysis_worker_, &AnalysisWorker::hrvTimeCompleted, this, [this](bool success, QString methodName, HRVTimeMetrics metrics, QString errorMessage) {
        if (success) {
            cached_hrv_metrics_ = metrics;
            hrv_time_completed_ = true;
            emit hrvTimeCompletedChanged();
            emit hrvTimeSuccess(methodName);
        } else {
            emit hrvTimeError(errorMessage);
        }
    });
    
    connect(analysis_worker_, &AnalysisWorker::hrvGeoCompleted, this, [this](bool success, HRVGeoMetrics metrics, QString errorMessage) {
        if (success) {
            cached_hrv_geo_metrics_ = metrics;
            hrv_geo_completed_ = true;
            emit hrvGeoCompletedChanged();
            emit hrvGeoSuccess();
        } else {
            emit hrvGeoError(errorMessage);
        }
    });
    
    connect(analysis_worker_, &AnalysisWorker::wavesCompleted, this, [this](bool success, QString errorMessage) {
        if (success) {
            waves_completed_ = true;
            emit wavesCompletedChanged();
            emit wavesSuccess();
        } else {
            emit wavesError(errorMessage);
        }
    });
    
    connect(analysis_worker_, &AnalysisWorker::heartClassCompleted, this, [this](bool success, HeartClassResult result, QString errorMessage) {
        if (success) {
            cached_heart_class_result_ = result;
            heart_class_completed_ = true;
            emit heartClassCompletedChanged();
            emit heartClassSuccess();
        } else {
            emit heartClassError(errorMessage);
        }
    });
    
    analysis_thread_->start();
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
        heart_class_completed_ = false;
        emit loadedFilenameChanged();
        emit isFileLoadedChanged();
        emit hasDataChanged();
        emit baselineCompletedChanged();
        emit rPeaksCompletedChanged();
        emit hrvTimeCompletedChanged();
        emit hrvGeoCompletedChanged();
        emit wavesCompletedChanged();
        emit heartClassCompletedChanged();
        emit fileLoadSuccess(filename);
    } else {
        QString errorMessage = application_service_->GetLastValidationError();
        if (errorMessage.isEmpty()) {
            errorMessage = "Nie udało się załadować pliku";
        }
        emit fileLoadError(errorMessage);
    }
}

void EkgController::openFileDialog() {
    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);

    while (!dir.exists("ludb") && dir.cdUp()) {
    }

    QString defaultPath = dir.absolutePath();

    QString filename = QFileDialog::getOpenFileName(
        nullptr,
        "Wybierz plik danych EKG",
        defaultPath,
        "DAT Files (*.dat);;All Files (*)"
    );

    if (!filename.isEmpty()) {
        loadData(filename);
    }
}

bool EkgController::runBaseline(int filterMethod, int windowSize, int polynomialOrder) {
    if (!hasData()) {
        emit filteringError("Nie załadowano danych. Najpierw zaimportuj sygnał.");
        return false;
    }

    FilterMethod method;
    switch (filterMethod) {
        case 0:
            method = MovingAverage;
            break;
        case 1:
            method = Butterworth;
            break;
        case 2:
            method = SavitzkyGolay;
            break;
        default:
            emit filteringError("Nieznany typ filtra");
            return false;
    }

    QMetaObject::invokeMethod(analysis_worker_, "runBaseline", Qt::QueuedConnection, Q_ARG(FilterMethod, method), Q_ARG(int, windowSize), Q_ARG(int, polynomialOrder));
    return true;
}

bool EkgController::runRPeaksDetection(int method) {
    if (!hasFilteredData()) {
        emit rPeaksDetectionError("Brak przefiltrowanych danych. Najpierw uruchom filtrowanie baseline.");
        return false;
    }

    RPeaksDetectionMethod rPeaksMethod;
    switch (method) {
        case 0:
            rPeaksMethod = PanTompkins;
            break;
        case 1:
            rPeaksMethod = Hilbert;
            break;
        case 2:
            rPeaksMethod = Wavelet;
            break;
        default:
            emit rPeaksDetectionError("Nieznana metoda detekcji");
            return false;
    }

    QMetaObject::invokeMethod(analysis_worker_, "runRPeaksDetection", Qt::QueuedConnection, Q_ARG(RPeaksDetectionMethod, rPeaksMethod));
    return true;
}

bool EkgController::runHRVTime(int method) {
    if (!rPeaksCompleted()) {
        emit hrvTimeError("Brak wykrytych pików R. Najpierw uruchom detekcję pików R.");
        return false;
    }

    HRVSpectralMethod spectralMethod;
    switch (method) {
        case 0:
            spectralMethod = ClassicPeriodogram;
            break;
        case 1:
            spectralMethod = LombScargle;
            break;
        case 2:
            spectralMethod = Welch;
            break;
        default:
            emit hrvTimeError("Nieznana metoda estymacji widma");
            return false;
    }

    QMetaObject::invokeMethod(analysis_worker_, "runHRVTime", Qt::QueuedConnection, Q_ARG(HRVSpectralMethod, spectralMethod));
    return true;
}

bool EkgController::runHRVGeo() {
    if (!rPeaksCompleted()) {
        emit hrvGeoError("Brak wykrytych pików R. Najpierw uruchom detekcję pików R.");
        return false;
    }

    QMetaObject::invokeMethod(analysis_worker_, "runHRVGeo", Qt::QueuedConnection);
    return true;
}

bool EkgController::runWaves() {
    if (!hasFilteredData()) {
        emit wavesError("Brak przefiltrowanych danych. Najpierw uruchom filtrowanie baseline.");
        return false;
    }

    QMetaObject::invokeMethod(analysis_worker_, "runWaves", Qt::QueuedConnection);
    return true;
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

bool EkgController::heartClassCompleted() const {
    return heart_class_completed_;
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

void EkgController::resetHeartClass() {
    heart_class_completed_ = false;
    cached_heart_class_result_ = HeartClassResult{};
    emit heartClassCompletedChanged();
}

bool EkgController::runHeartClass() {
    if (!hasFilteredData()) {
        emit heartClassError("Brak przefiltrowanych danych. Najpierw uruchom filtrowanie baseline.");
        return false;
    }

    QMetaObject::invokeMethod(analysis_worker_, "runHeartClass", Qt::QueuedConnection);
    return true;
}

QVariantList EkgController::getHeartClassAnnotations() const {
    QVariantList result;
    
    const auto filtered = application_service_->GetFilteredData();
    if (!filtered || filtered->values.empty()) return result;
    
    const double frequency = filtered->frequency > 0 ? static_cast<double>(filtered->frequency) : 1.0;
    
    for (const auto& [sampleIdx, label] : cached_heart_class_result_.annotations) {
        QVariantMap entry;
        entry["sample"] = sampleIdx;
        entry["time"] = static_cast<double>(sampleIdx) / frequency;
        entry["label"] = QString::fromStdString(label);
        result.append(entry);
    }
    
    return result;
}

QVariantMap EkgController::getHeartClassBarChart() const {
    QVariantMap result;
    
    int countN = 0, countV = 0, countA = 0, countOther = 0;
    int total = static_cast<int>(cached_heart_class_result_.annotations.size());
    
    for (const auto& [sampleIdx, label] : cached_heart_class_result_.annotations) {
        if (label == "N") {
            countN++;
        } else if (label == "V") {
            countV++;
        } else if (label == "A") {
            countA++;
        } else {
            countOther++;
        }
    }
    
    result["N"] = countN;
    result["V"] = countV;
    result["A"] = countA;
    result["Other"] = countOther;
    result["total"] = total;
    result["percentN"] = total > 0 ? (countN * 100.0 / total) : 0.0;
    result["percentV"] = total > 0 ? (countV * 100.0 / total) : 0.0;
    result["percentA"] = total > 0 ? (countA * 100.0 / total) : 0.0;
    result["percentOther"] = total > 0 ? (countOther * 100.0 / total) : 0.0;
    
    return result;
}

QStringList EkgController::getAvailableFiles() const {
    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);

    while (!dir.exists("ludb") && dir.cdUp()) {
    }

    QStringList fileBasenames;
    QStringList filters;
    filters << "*.dat";

    QString ludbPath = dir.absoluteFilePath("ludb");
    QDir ludbDir(ludbPath);
    if (ludbDir.exists()) {
        ludbDir.setNameFilters(filters);
        ludbDir.setSorting(QDir::Name);
        QStringList ludbFiles = ludbDir.entryList(QDir::Files);
        for (const QString &file : ludbFiles) {
            QFileInfo fileInfo(file);
            fileBasenames.append("LUDB/" + fileInfo.completeBaseName());
        }
    }

    QString mitbihPath = dir.absoluteFilePath("mitbih");
    QDir mitbihDir(mitbihPath);
    if (mitbihDir.exists()) {
        mitbihDir.setNameFilters(filters);
        mitbihDir.setSorting(QDir::Name);
        QStringList mitbihFiles = mitbihDir.entryList(QDir::Files);
        for (const QString &file : mitbihFiles) {
            QFileInfo fileInfo(file);
            fileBasenames.append("MITBIH/" + fileInfo.completeBaseName());
        }
    }

    return fileBasenames;
}

void EkgController::loadFileByName(const QString &filename) {
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

    loadData(fullPath);
}

void EkgController::resetBaseline() {
    application_service_->ClearFilteredData();
    baseline_completed_ = false;
    r_peaks_completed_ = false;
    hrv_time_completed_ = false;
    hrv_geo_completed_ = false;
    waves_completed_ = false;
    heart_class_completed_ = false;
    emit hasFilteredDataChanged();
    emit baselineCompletedChanged();
    emit rPeaksCompletedChanged();
    emit hrvTimeCompletedChanged();
    emit hrvGeoCompletedChanged();
    emit wavesCompletedChanged();
    emit heartClassCompletedChanged();
}

void EkgController::resetRPeaks() {
    application_service_->ClearRPeaks();
    r_peaks_completed_ = false;
    hrv_time_completed_ = false;
    hrv_geo_completed_ = false;
    waves_completed_ = false;
    heart_class_completed_ = false;
    emit rPeaksCompletedChanged();
    emit hrvTimeCompletedChanged();
    emit hrvGeoCompletedChanged();
    emit wavesCompletedChanged();
    emit heartClassCompletedChanged();
}

namespace {
QVariantList buildSeries(const std::shared_ptr<SignalDataset> &dataset, int channel, int maxPoints, double startTime, double endTime) {
    QVariantList series;
    if (!dataset || dataset->values.empty()) return series;

    const int channelCount = static_cast<int>(dataset->values.front().channelValues.size());
    if (channelCount == 0) return series;

    const int clampedChannel = std::max(0, std::min(channel, channelCount - 1));
    const double frequency = dataset->frequency > 0 ? static_cast<double>(dataset->frequency) : 1.0;

    const int totalSampleCount = static_cast<int>(dataset->values.size());
    
    int startIdx = 0;
    int endIdx = totalSampleCount;
    
    if (startTime >= 0 && endTime > startTime) {
        startIdx = std::max(0, static_cast<int>(startTime * frequency));
        endIdx = std::min(totalSampleCount, static_cast<int>(endTime * frequency) + 1);
    }
    
    const int rangeSize = endIdx - startIdx;
    int stride = 1;
    if (maxPoints > 0) {
        stride = std::max(1, static_cast<int>(std::ceil(static_cast<double>(rangeSize) / maxPoints)));
    }

    series.reserve(rangeSize / stride + 1);

    for (int i = startIdx; i < endIdx; i += stride) {
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

QVariantList EkgController::getRawSeries(int channel, int maxPoints, double startTime, double endTime) const {
    return buildSeries(application_service_->GetData(), channel, maxPoints, startTime, endTime);
}

QVariantList EkgController::getFilteredSeries(int channel, int maxPoints, double startTime, double endTime) const {
    return buildSeries(application_service_->GetFilteredData(), channel, maxPoints, startTime, endTime);
}

QVariantList EkgController::getRPeakMarkers(int channel, double startTime, double endTime) const {
    QVariantList markers;

    const auto peaks = application_service_->GetRPeaks();
    if (!peaks || peaks->empty()) return markers;

    const auto filtered = application_service_->GetFilteredData();
    if (!filtered || filtered->values.empty()) return markers;

    const int channelCount = static_cast<int>(filtered->values.front().channelValues.size());
    if (channelCount == 0) return markers;

    const int clampedChannel = std::max(0, std::min(channel, channelCount - 1));
    const double frequency = filtered->frequency > 0 ? static_cast<double>(filtered->frequency) : 1.0;

    const int totalSamples = static_cast<int>(peaks->size());
    int startIdx = 0;
    int endIdx = totalSamples;
    
    if (startTime >= 0 && endTime > startTime) {
        startIdx = std::max(0, static_cast<int>(startTime * frequency));
        endIdx = std::min(totalSamples, static_cast<int>(endTime * frequency) + 1);
    }

    for (int i = startIdx; i < endIdx; ++i) {
        const auto &peak = (*peaks)[i];
        if (static_cast<size_t>(clampedChannel) >= peak.peaks.size() || !peak.peaks[static_cast<size_t>(clampedChannel)]) {
            continue;
        }
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
    metrics["nn50"] = cached_hrv_metrics_.nn50;
    metrics["pnn50"] = static_cast<double>(cached_hrv_metrics_.pnn50);
    metrics["tp"] = static_cast<double>(cached_hrv_metrics_.tp);
    metrics["vlf"] = static_cast<double>(cached_hrv_metrics_.vlf);
    metrics["lf"] = static_cast<double>(cached_hrv_metrics_.lf);
    metrics["hf"] = static_cast<double>(cached_hrv_metrics_.hf);
    metrics["lf_hf"] = static_cast<double>(cached_hrv_metrics_.lf_hf);
    return metrics;
}

QVariantList EkgController::getHRVTimePowerSpectrum() const {
    QVariantList result;
    const auto& power = cached_hrv_metrics_.power_spectrum;
    const auto& freqs = cached_hrv_metrics_.frequencies;
    
    for (size_t i = 0; i < power.size() && i < freqs.size(); ++i) {
        QVariantMap entry;
        entry["x"] = freqs[i];
        entry["y"] = power[i];
        result.append(entry);
    }
    return result;
}

QVariantList EkgController::getHRVTimeTachogram() const {
    QVariantList result;
    const auto& rr = cached_hrv_metrics_.rr_intervals;
    const auto& times = cached_hrv_metrics_.tachogram_times;
    
    for (size_t i = 0; i < rr.size() && i + 1 < times.size(); ++i) {
        QVariantMap entry;
        entry["x"] = times[i];
        entry["y"] = rr[i];
        result.append(entry);
    }
    return result;
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
        const size_t ch = static_cast<size_t>(clampedChannel);

        if (ch < wave.p_wave_onset.size() && wave.p_wave_onset[ch]) {
            QVariantMap entry;
            entry["x"] = t;
            entry["y"] = y;
            pOnsets.append(entry);
        }
        if (ch < wave.p_wave_end.size() && wave.p_wave_end[ch]) {
            QVariantMap entry;
            entry["x"] = t;
            entry["y"] = y;
            pEnds.append(entry);
        }
        if (ch < wave.qrs_onset.size() && wave.qrs_onset[ch]) {
            QVariantMap entry;
            entry["x"] = t;
            entry["y"] = y;
            qrsOnsets.append(entry);
        }
        if (ch < wave.qrs_end.size() && wave.qrs_end[ch]) {
            QVariantMap entry;
            entry["x"] = t;
            entry["y"] = y;
            qrsEnds.append(entry);
        }
        if (ch < wave.t_end.size() && wave.t_end[ch]) {
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

double EkgController::signalDuration() const {
    auto data = application_service_->GetData();
    if (data && !data->values.empty() && data->frequency > 0) {
        return static_cast<double>(data->values.size()) / static_cast<double>(data->frequency);
    }

    auto filtered = application_service_->GetFilteredData();
    if (filtered && !filtered->values.empty() && filtered->frequency > 0) {
        return static_cast<double>(filtered->values.size()) / static_cast<double>(filtered->frequency);
    }

    return 0.0;
}

bool EkgController::exportFilteredSignal(int format, const QString &filepath) {
    FileFormat fileFormat = static_cast<FileFormat>(format);
    return results_repository_->ExportFilteredSignal(
        fileFormat,
        filepath,
        loadedFilename(),
        application_service_->GetFilteredData()
    );
}

bool EkgController::exportRPeaks(int format, const QString &filepath) {
    FileFormat fileFormat = static_cast<FileFormat>(format);
    return results_repository_->ExportRPeaks(
        fileFormat,
        filepath,
        loadedFilename(),
        application_service_->GetRPeaks(),
        application_service_->GetFilteredData()
    );
}

bool EkgController::exportHRVTime(int format, const QString &filepath) {
    FileFormat fileFormat = static_cast<FileFormat>(format);
    return results_repository_->ExportHRVTime(
        fileFormat,
        filepath,
        loadedFilename(),
        cached_hrv_metrics_
    );
}

bool EkgController::exportHRVGeo(int format, const QString &filepath) {
    FileFormat fileFormat = static_cast<FileFormat>(format);
    return results_repository_->ExportHRVGeo(
        fileFormat,
        filepath,
        loadedFilename(),
        cached_hrv_geo_metrics_
    );
}

bool EkgController::exportWaves(int format, const QString &filepath) {
    FileFormat fileFormat = static_cast<FileFormat>(format);
    return results_repository_->ExportWaves(
        fileFormat,
        filepath,
        loadedFilename(),
        application_service_->GetWaves(),
        application_service_->GetFilteredData()
    );
}

bool EkgController::exportHeartClass(int format, const QString &filepath) {
    FileFormat fileFormat = static_cast<FileFormat>(format);
    return results_repository_->ExportHeartClass(
        fileFormat,
        filepath,
        loadedFilename(),
        cached_heart_class_result_,
        samplingFrequency()
    );
}

void EkgController::openSaveSettingsDialog(int selectedFilterMethod, int selectedRPeaksMethod, int selectedHRVTimeMethod, int selectedChannelIndex, const QString &currentModule, bool isDarkTheme, double chartWindowSize, int maxPlottedPoints, int windowSize, int polynomialOrder) {
    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);
    
    while (!dir.exists("ludb") && dir.cdUp()) {
    }
    
    QString defaultPath = dir.absolutePath();
    QString filename = QFileDialog::getSaveFileName(
        nullptr,
        "Zapisz ustawienia",
        defaultPath + "/ekg_settings.json",
        "JSON files (*.json);;All Files (*)"
    );
    
    if (filename.isEmpty()) {
        return;
    }
    
    QJsonObject settings;
    settings["selectedFilterMethod"] = selectedFilterMethod;
    settings["selectedRPeaksMethod"] = selectedRPeaksMethod;
    settings["selectedHRVTimeMethod"] = selectedHRVTimeMethod;
    settings["selectedChannel"] = selectedChannelIndex;
    settings["currentModule"] = currentModule;
    settings["isDarkTheme"] = isDarkTheme;
    settings["chartWindowSize"] = chartWindowSize;
    settings["maxPlottedPoints"] = maxPlottedPoints;
    settings["windowSize"] = windowSize;
    settings["polynomialOrder"] = polynomialOrder;
    
    QJsonDocument doc(settings);
    QFile file(filename);
    
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit settingsSaveError("Nie można otworzyć pliku do zapisu");
        return;
    }
    
    file.write(doc.toJson());
    file.close();
    
    emit settingsSaveSuccess(filename);
}

void EkgController::openLoadSettingsDialog() {
    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);
    
    while (!dir.exists("ludb") && dir.cdUp()) {
    }
    
    QString defaultPath = dir.absolutePath();
    QString filename = QFileDialog::getOpenFileName(
        nullptr,
        "Załaduj ustawienia",
        defaultPath,
        "JSON files (*.json);;All Files (*)"
    );
    
    if (filename.isEmpty()) {
        return;
    }
    
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit settingsLoadError("Nie można otworzyć pliku do odczytu");
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        emit settingsLoadError("Błąd parsowania JSON: " + error.errorString());
        return;
    }
    
    if (!doc.isObject()) {
        emit settingsLoadError("Nieprawidłowy format pliku ustawień");
        return;
    }
    
    QJsonObject settings = doc.object();
    QVariantMap settingsMap;
    
    for (auto it = settings.begin(); it != settings.end(); ++it) {
        settingsMap[it.key()] = it.value().toVariant();
    }
    
    emit settingsLoadSuccess(settingsMap);
}
