#ifndef EKG_HRV_GEO_PROCESSING_SERVICE_H
#define EKG_HRV_GEO_PROCESSING_SERVICE_H
#include "../dto/hrv_geo_metrics.h"
#include "../model/signal_datapoint.h"

class IHRVGeoProcessingService {
public:
    virtual ~IHRVGeoProcessingService() = default;

    virtual HRVGeoMetrics Process(std::vector<SignalDatapoint> datapoints, int frequency) = 0;
};

#endif //EKG_HRV_GEO_PROCESSING_SERVICE_H
