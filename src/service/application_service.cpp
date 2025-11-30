#include "../../include/service/application_service.h"

#include <iostream>
#include <QFileInfo>

#include "../../include/dto/filter_method.h"
#include "../../include/service/r_peaks_detection_service.h"

ApplicationService::ApplicationService(
    std::shared_ptr<ISignalRepository> signal_repository,
    std::shared_ptr<IFilterService> butterworth_filter_service,
    std::shared_ptr<IFilterService> moving_average_filter_service,
    std::shared_ptr<IRPeaksDetectionService> r_peaks_detection_service,
    std::shared_ptr<IHRVTimeProcessingService> hrv_time_processing_service,
    std::shared_ptr<IHRVGeoProcessingService> hrv_geo_processing_service,
    std::shared_ptr<IHRVDFAProcessingService> hrv_dfa_processing_service,
    std::shared_ptr<IWavesDetectionService> waves_detection_service,
    std::shared_ptr<IHeartClassDetectionService> heart_class_detection_service
)
    : signal_repository_(std::move(signal_repository)),
      butterworth_filter_service_(std::move(butterworth_filter_service)),
      moving_average_filter_service_(std::move(moving_average_filter_service)),
      r_peaks_detection_service_(std::move(r_peaks_detection_service)),
      hrv_time_processing_service_(std::move(hrv_time_processing_service)),
      hrv_geo_processing_service_(std::move(hrv_geo_processing_service)),
      hrv_dfa_processing_service_(std::move(hrv_dfa_processing_service)),
      heart_class_detection_service_(std::move(heart_class_detection_service)),
      waves_detection_service_(std::move(waves_detection_service)) {
}

bool ApplicationService::Load(const QString &filename) {
    this->loaded_dataset = signal_repository_->Load(filename);
    const QFileInfo fileInfo(filename);
    this->loaded_filename = fileInfo.completeBaseName();
    return true;
}

std::shared_ptr<SignalDataset> ApplicationService::GetData() const {
    return this->loaded_dataset;
}

std::shared_ptr<SignalDataset> ApplicationService::GetFilteredData() const {
    return this->filtered_dataset;
}

bool ApplicationService::IsFileLoaded() const {
    return loaded_filename.isEmpty();
}

QString ApplicationService::GetLoadedFilename() const {
    return loaded_filename;
}

std::shared_ptr<std::vector<RPeaksAnnotatedSignalDatapoint> > ApplicationService::GetRPeaks() const {
    return this->r_peaks;
}

bool ApplicationService::RunFiltering(FilterMethod method) const {
    if (this->loaded_dataset == nullptr) return false;
    if (method == Butterworth) {
        this->filtered_dataset->values = this->butterworth_filter_service_->Filter(this->loaded_dataset->values);
    } else if (method == MovingAverage) {
        this->moving_average_filter_service_->Filter(this->loaded_dataset->values);
    }
    return true;
}

bool ApplicationService::CalculateRPeaks(RPeaksDetectionMethod method) const {
    if (filtered_dataset == nullptr) return false;
    this->r_peaks_detection_service_->Detect(this->filtered_dataset->values, this->filtered_dataset->frequency, method);
    return true;
}
