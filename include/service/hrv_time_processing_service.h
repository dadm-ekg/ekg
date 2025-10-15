#ifndef EKG_HRV_TIME_PROCESSING_SERVICE_H
#define EKG_HRV_TIME_PROCESSING_SERVICE_H
#include <vector>

#include "../dto/hrv_time_metrics.h"
#include "../model/signal_datapoint.h"

class IHRVTimeProcessingService {
public:
    virtual ~IHRVTimeProcessingService() = default;

    virtual HRVTimeMetrics Process(std::vector<SignalDatapoint> datapoints, int frequency) = 0;
};

#endif //EKG_HRV_TIME_PROCESSING_SERVICE_H
