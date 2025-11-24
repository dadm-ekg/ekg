#include "../../include/service/application_service.h"

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
    const auto dataset = signal_repository_->Load(filename);
    const auto filtered_signal_dataset = butterworth_filter_service_->Filter(dataset->values);
    // Tymczasowo, po prostu uruchamiamy filtr butterwortha i uruchamiamy kolejne moduły. Docelowo będzie od tego przycisk, który podepnie się na końcu.
    // moving_average_filter_service_->Filter(dataset->values);
    hrv_time_processing_service_->Process(filtered_signal_dataset, dataset->frequency);
    r_peaks_detection_service_->Detect(filtered_signal_dataset, dataset->frequency);
    hrv_time_processing_service_->Process(filtered_signal_dataset, dataset->frequency);
    hrv_dfa_processing_service_->Process(filtered_signal_dataset, dataset->frequency);
    heart_class_detection_service_->Detect(filtered_signal_dataset, dataset->frequency);
    waves_detection_service_->Detect(filtered_signal_dataset, dataset->frequency);
    // TODO(Mati W.): trzeba uzupełnić
    return true;
}

int ApplicationService::GetLength() const {
    // TODO(Mati W.): trzeba uzupełnić
}

SignalRange ApplicationService::GetViewRange() const {
    // TODO(Mati W.): trzeba uzupełnić
}

void ApplicationService::SetViewRange(SignalRange range) {
    // TODO(Mati W.): trzeba uzupełnić
}

std::shared_ptr<std::vector<SignalDatapoint> > ApplicationService::GetData() const {
    // TODO(Mati W.): trzeba uzupełnić
}

std::shared_ptr<std::vector<SignalDatapoint> > ApplicationService::GetFilteredData() const {
    // TODO(Mati W.): trzeba uzupełnić
}

Status ApplicationService::GetStatus() const {
    // TODO(Mati W.): trzeba uzupełnić
}

bool ApplicationService::RunFiltering(FilterMethod method) {
    // TODO(Mati W.): trzeba uzupełnić
}

int ApplicationService::GetFrequency() const {
    // TODO(Mati W.): trzeba uzupełnić
}
