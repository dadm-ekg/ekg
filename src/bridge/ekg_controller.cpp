#include "../../include/bridge/ekg_controller.h"
#include "../../include/dto/r_peaks_detection_method.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QPointF>
#include <QVariant>
#include <algorithm>
#include <cmath>

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
        emit loadedFilenameChanged();
        emit isFileLoadedChanged();
        emit hasDataChanged();
        emit baselineCompletedChanged();
        emit rPeaksCompletedChanged();
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
        emit hasFilteredDataChanged();
        emit baselineCompletedChanged();
        emit rPeaksCompletedChanged();
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
        emit rPeaksCompletedChanged();
        emit rPeaksDetectionSuccess(methodName);
    } else {
        emit rPeaksDetectionError("Nie udało się wykryć pików R metodą " + methodName);
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
    emit hasFilteredDataChanged();
    emit baselineCompletedChanged();
    emit rPeaksCompletedChanged();
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
} // namespace

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

