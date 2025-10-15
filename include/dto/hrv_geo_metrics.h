#ifndef EKG_HRV_GEO_METRICS_H
#define EKG_HRV_GEO_METRICS_H
#include <vector>

class HRVGeoMetrics {
public:
    float triangular_index;
    float tinn;
    std::vector<float> histogram;
};

#endif //EKG_HRV_GEO_METRICS_H