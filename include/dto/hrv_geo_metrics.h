#ifndef EKG_HRV_GEO_METRICS_H
#define EKG_HRV_GEO_METRICS_H
#include <vector>

class HRVGeoMetrics {
public:
    double triangular_index = 0.0;
    double tinn = 0.0;
    std::vector<double> histogram;
    double sd1 = 0.0;
    double sd2 = 0.0;
    double rr_min = 0.0;
    double bin_width = 5.0;
    std::vector<double> rr_intervals;
};

#endif //EKG_HRV_GEO_METRICS_H