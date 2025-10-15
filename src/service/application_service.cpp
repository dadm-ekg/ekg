#include "../../include/service/application_service.h"

#include <qstring.h>

#include "../../include/repository/signal_repository.h"
#include "../../include/service/filter_service.h"
#include "../../include/service/r_peaks_detection_service.h"

class ApplicationService : public IApplicationService {
    std::shared_ptr<ISignalRepository> signal_repository_;
    std::shared_ptr<IFilterService> butterworth_filter_service_;
    std::shared_ptr<IFilterService> moving_average_filter_service_;
    std::shared_ptr<IRPeaksDetectionService> r_peaks_detection_service_;

public:
    explicit ApplicationService(
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

    bool Load(QString filename) override {
        signal_repository_->Load(filename);
    }

    int GetLength() override {
    }

    SignalRange GetViewRange() override {
    }

    void SetViewRange(SignalRange range) override {
    }

    std::shared_ptr<std::vector<SignalDatapoint> > GetData() override {
    }

    std::shared_ptr<std::vector<SignalDatapoint> > GetFilteredData() override {
    }

    Status GetStatus() override {
    }

    bool RunFiltering(FilterMethod method) override {
    }

    int GetFrequency() override {};
};
