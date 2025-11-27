#ifndef EKG_HRV_GEO_PROCESSING_SERVICE_H
#define EKG_HRV_GEO_PROCESSING_SERVICE_H
#include "../../dto/hrv_geo_metrics.h"
#include <vector>

class IHRVGeoProcessingService {
public:
    virtual ~IHRVGeoProcessingService() = default;
    virtual HRVGeoMetrics Process(const std::vector<double>& rr_intervals) = 0;
};

#endif //EKG_HRV_GEO_PROCESSING_SERVICE_H

