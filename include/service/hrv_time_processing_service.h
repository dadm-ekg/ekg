#ifndef EKG_HRV_TIME_PROCESSING_SERVICE_IMPL_H
#define EKG_HRV_TIME_PROCESSING_SERVICE_IMPL_H

#include "abstract/hrv_time_processing_service.h"

class HRVTimeProcessingService : public IHRVTimeProcessingService {
public:
    HRVTimeMetrics Process(
        const std::vector<SignalDatapoint>& datapoints,
        const std::vector<RPeaksAnnotatedSignalDatapoint>& r_peaks,
        int frequency,
        HRVTimeMetrics::SpectralMethod method = HRVTimeMetrics::SpectralMethod::CLASSIC_PERIODOGRAM
    ) override;
};

#endif //EKG_HRV_TIME_PROCESSING_SERVICE_IMPL_H
