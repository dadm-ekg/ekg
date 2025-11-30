#ifndef EKG_APPLICATION_SERVICE_IMPL_H
#define EKG_APPLICATION_SERVICE_IMPL_H

#include "abstract/application_service.h"
#include "../repository/abstract/signal_repository.h"
#include "abstract/filter_service.h"
#include "abstract/r_peaks_detection_service.h"
#include "abstract/hrv_time_processing_service.h"
#include "abstract/hrv_geo_processing_service.h"
#include "abstract/hrv_dfa_processing_service.h"
#include "abstract/waves_detection_service.h"
#include "abstract/heart_class_detection_service.h"
#include "../dto/filter_method.h"

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

    QString loaded_filename;
    std::shared_ptr<SignalDataset> loaded_dataset;
    mutable std::shared_ptr<SignalDataset> filtered_dataset;
    mutable std::shared_ptr<std::vector<RPeaksAnnotatedSignalDatapoint> > r_peaks;

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
    );

    bool Load(const QString &filename) override;

    std::shared_ptr<SignalDataset> GetData() const override;

    std::shared_ptr<SignalDataset> GetFilteredData() const override;

    bool IsFileLoaded() const override;

    QString GetLoadedFilename() const override;

    std::shared_ptr<std::vector<RPeaksAnnotatedSignalDatapoint> > GetRPeaks() const override;

    bool RunFiltering(FilterMethod method) const override;

    bool CalculateRPeaks(RPeaksDetectionMethod method) const override;
};

#endif //EKG_APPLICATION_SERVICE_IMPL_H
