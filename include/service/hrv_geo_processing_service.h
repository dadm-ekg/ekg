#ifndef EKG_HRV_GEO_PROCESSING_SERVICE_IMPL_H
#define EKG_HRV_GEO_PROCESSING_SERVICE_IMPL_H

#include "abstract/hrv_geo_processing_service.h"

class HRVGeoProcessingService : public IHRVGeoProcessingService {
public:
    HRVGeoMetrics Process(const std::vector<SignalDatapoint>& datapoints, int frequency) override;
};

#endif //EKG_HRV_GEO_PROCESSING_SERVICE_IMPL_H
