#include "../../include/service/application_service.h"

#include <qstring.h>

#include "../../include/repository/signal_repository.h"
#include "../../include/service/filter_service.h"
#include "../../include/service/r_peaks_detection_service.h"
#include "../../include/service/hrv_time_processing_service.h"
#include "../../include/service/hrv_geo_processing_service.h"
#include "../../include/service/hrv_dfa_processing_service.h"
#include "../../include/service/waves_detection_service.h"
#include "../../include/service/heart_class_detection_service.h"

class ApplicationService : public IApplicationService {
    std::shared_ptr<ISignalRepository> signal_repository_;
    std::shared_ptr<IFilterService> butterworth_filter_service_;
    std::shared_ptr<IFilterService> moving_average_filter_service_;
    std::shared_ptr<IRPeaksDetectionService> r_peaks_detection_service_;
    std::shared_ptr<IHRVTimeProcessingService> hrv_time_processing_service_;
    std::shared_ptr<IHRVGeoProcessingService> hrv_geo_processing_service_;
    std::shared_ptr<IHRVDFAProcessingService> hrv_dfa_processing_service_;
    std::shared_ptr<IHeartClassDetectionService> heart_class_detection_service_;
    std::shared_ptr<IWavesDetectionService> waves_detection_service_;

public:
    explicit ApplicationService(
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

    bool Load(QString filename) override {
        signal_repository_->Load(filename);
    }

    int GetLength() override {
        // TODO(Mati W.): trzeba uzupełnić
    }

    SignalRange GetViewRange() override {
        // TODO(Mati W.): trzeba uzupełnić
    }

    void SetViewRange(SignalRange range) override {
        // TODO(Mati W.): trzeba uzupełnić
    }

    std::shared_ptr<std::vector<SignalDatapoint> > GetData() override {
        // TODO(Mati W.): trzeba uzupełnić
    }

    std::shared_ptr<std::vector<SignalDatapoint> > GetFilteredData() override {
        // TODO(Mati W.): trzeba uzupełnić
    }

    Status GetStatus() override {
        // TODO(Mati W.): trzeba uzupełnić
    }

    bool RunFiltering(FilterMethod method) override {
        // TODO(Mati W.): trzeba uzupełnić
    }

    int GetFrequency() override {
        // TODO(Mati W.): trzeba uzupełnić
    };
};
