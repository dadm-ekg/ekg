#include "../../include/service/application_service.h"

#include <iostream>
#include <QFileInfo>

#include "../../include/dto/filter_method.h"
#include "../../include/service/r_peaks_detection_service.h"

ApplicationService::ApplicationService(
    std::shared_ptr<ISignalRepository> signal_repository,
    std::shared_ptr<IFilterService> butterworth_filter_service,
    std::shared_ptr<IFilterService> moving_average_filter_service,
    std::shared_ptr<IRPeaksDetectionService> r_peaks_detection_service
)
    : signal_repository_(std::move(signal_repository)),
      butterworth_filter_service_(std::move(butterworth_filter_service)),
      moving_average_filter_service_(std::move(moving_average_filter_service)),
      r_peaks_detection_service_(std::move(r_peaks_detection_service)) {
}

bool ApplicationService::Load(const QString &filename) {
    this->loaded_dataset = signal_repository_->Load(filename);
    this->filtered_dataset = nullptr;
    this->r_peaks = nullptr;
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
    return !loaded_filename.isEmpty();
}

QString ApplicationService::GetLoadedFilename() const {
    return loaded_filename;
}

std::shared_ptr<std::vector<RPeaksAnnotatedSignalDatapoint> > ApplicationService::GetRPeaks() const {
    return this->r_peaks;
}

bool ApplicationService::RunFiltering(FilterMethod method) const {
    if (this->loaded_dataset == nullptr) return false;
    
    this->filtered_dataset = std::make_shared<SignalDataset>();
    this->filtered_dataset->frequency = this->loaded_dataset->frequency;
    this->r_peaks = nullptr;
    
    if (method == Butterworth) {
        this->filtered_dataset->values = this->butterworth_filter_service_->Filter(this->loaded_dataset->values);
    } else if (method == MovingAverage) {
        this->filtered_dataset->values = this->moving_average_filter_service_->Filter(this->loaded_dataset->values);
    }
    
    return true;
}

void ApplicationService::ClearFilteredData() const {
    this->filtered_dataset = nullptr;
    this->r_peaks = nullptr;
}

void ApplicationService::ClearRPeaks() const {
    this->r_peaks = nullptr;
}

bool ApplicationService::CalculateRPeaks(RPeaksDetectionMethod method) const {
    if (filtered_dataset == nullptr) return false;
    
    auto detected_peaks = this->r_peaks_detection_service_->Detect(
        this->filtered_dataset->values, 
        this->filtered_dataset->frequency, 
        method
    );
    
    this->r_peaks = std::make_shared<std::vector<RPeaksAnnotatedSignalDatapoint>>(detected_peaks);
    
    return true;
}
