#include "../../include/service/application_service.h"

#include <iostream>
#include <QFileInfo>

#include "../../include/dto/filter_method.h"
#include "../../include/service/r_peaks_detection_service.h"
#include "../../include/repository/dat_signal_repository.h"
#include "../../include/dto/validation_result.h"

ApplicationService::ApplicationService(
    std::shared_ptr<ISignalRepository> signal_repository,
    std::shared_ptr<IFilterService> butterworth_filter_service,
    std::shared_ptr<IFilterService> moving_average_filter_service,
    std::shared_ptr<IFilterService> savitzky_golay_filter_service,
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
      savitzky_golay_filter_service_(std::move(savitzky_golay_filter_service)),
      r_peaks_detection_service_(std::move(r_peaks_detection_service)),
      hrv_time_processing_service_(std::move(hrv_time_processing_service)),
      hrv_geo_processing_service_(std::move(hrv_geo_processing_service)),
      hrv_dfa_processing_service_(std::move(hrv_dfa_processing_service)),
      waves_detection_service_(std::move(waves_detection_service)),
      heart_class_detection_service_(std::move(heart_class_detection_service)) {
}

bool ApplicationService::Load(const QString &filename) {
    last_validation_error_ = "";
    
    this->loaded_dataset = signal_repository_->Load(filename);
    
    auto* datRepo = dynamic_cast<DATSignalRepository*>(signal_repository_.get());
    if (datRepo) {
        ValidationResult validation = datRepo->GetLastValidationResult();
        if (!validation.isValid) {
            last_validation_error_ = validation.getFormattedErrors();
        }
    }
    
    if (!this->loaded_dataset || this->loaded_dataset->values.empty()) {
        this->loaded_dataset = nullptr;
        this->filtered_dataset = nullptr;
        this->r_peaks = nullptr;
        this->waves = nullptr;
        this->heart_class_result_ = HeartClassResult{};
        this->loaded_filename = "";
        if (last_validation_error_.isEmpty()) {
            last_validation_error_ = "Nie udało się załadować danych z pliku";
        }
        return false;
    }
    
    this->filtered_dataset = nullptr;
    this->r_peaks = nullptr;
    this->waves = nullptr;
    this->heart_class_result_ = HeartClassResult{};
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

std::shared_ptr<std::vector<WaveAnnotatedSignalDatapoint> > ApplicationService::GetWaves() const {
    return this->waves;
}

bool ApplicationService::RunFiltering(FilterMethod method, int windowSize, int polynomialOrder) const {
    if (this->loaded_dataset == nullptr) return false;
    
    this->filtered_dataset = std::make_shared<SignalDataset>();
    this->filtered_dataset->frequency = this->loaded_dataset->frequency;
    this->r_peaks = nullptr;
    this->waves = nullptr;
    this->heart_class_result_ = HeartClassResult{};
    
    if (method == Butterworth) {
        this->filtered_dataset->values = this->butterworth_filter_service_->Filter(this->loaded_dataset->values, windowSize, polynomialOrder);
    } else if (method == MovingAverage) {
        this->filtered_dataset->values = this->moving_average_filter_service_->Filter(this->loaded_dataset->values, windowSize, polynomialOrder);
    } else if (method == SavitzkyGolay) {
        this->filtered_dataset->values = this->savitzky_golay_filter_service_->Filter(this->loaded_dataset->values, windowSize, polynomialOrder);
    } else {
        return false;
    }
    
    if (this->filtered_dataset->values.empty()) {
        return false;
    }
    
    return true;
}

void ApplicationService::ClearFilteredData() const {
    this->filtered_dataset = nullptr;
    this->r_peaks = nullptr;
    this->waves = nullptr;
    this->heart_class_result_ = HeartClassResult{};
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

HRVTimeMetrics ApplicationService::CalculateHRVTime(HRVTimeMetrics::SpectralMethod method) const {
    if (filtered_dataset == nullptr || r_peaks == nullptr) {
        return HRVTimeMetrics{};
    }
    return hrv_time_processing_service_->Process(
        filtered_dataset->values, 
        *r_peaks, 
        filtered_dataset->frequency, 
        method
    );
}

HRVGeoMetrics ApplicationService::CalculateHRVGeo() const {
    if (filtered_dataset == nullptr || r_peaks == nullptr) {
        return HRVGeoMetrics{};
    }
    return hrv_geo_processing_service_->Process(
        *r_peaks,
        filtered_dataset->frequency
    );
}

HRVDFAMetrics ApplicationService::CalculateHRVDFA() const {
    if (filtered_dataset == nullptr || r_peaks == nullptr) {
        return HRVDFAMetrics{};
    }
    return hrv_dfa_processing_service_->Process(
        *r_peaks,
        filtered_dataset->frequency
    );
}

bool ApplicationService::CalculateWaves() const {
    if (filtered_dataset == nullptr) return false;
    
    auto detected_waves = waves_detection_service_->Detect(
        filtered_dataset->values,
        filtered_dataset->frequency
    );
    
    this->waves = std::make_shared<std::vector<WaveAnnotatedSignalDatapoint>>(detected_waves);
    
    return true;
}

HeartClassResult ApplicationService::CalculateHeartClass() const {
    if (filtered_dataset == nullptr || r_peaks == nullptr) {
        return HeartClassResult{};
    }
    
    heart_class_result_ = heart_class_detection_service_->Detect(
        filtered_dataset->values,
        *r_peaks,
        filtered_dataset->frequency
    );
    
    return heart_class_result_;
}

QString ApplicationService::GetLastValidationError() const {
    return last_validation_error_;
}
