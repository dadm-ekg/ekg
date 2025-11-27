#include "../../include/service/hrv_geo_processing_service.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

HRVGeoMetrics HRVGeoProcessingService::Process(const std::vector<double>& rr_intervals) {
    HRVGeoMetrics metrics;

    if (rr_intervals.size() < 2) {
        return metrics;
    }

    // HISTOGRAM RR
    double min_rr = *std::min_element(rr_intervals.begin(), rr_intervals.end());
    double max_rr = *std::max_element(rr_intervals.begin(), rr_intervals.end());

    const double bin_width = 5.0;
    int num_bins = static_cast<int>(std::ceil((max_rr - min_rr) / bin_width)) + 1;

    std::vector<double> histogram(num_bins, 0.0);
    for (double interval : rr_intervals) {
        int bin = static_cast<int>((interval - min_rr) / bin_width);
        if (bin >= 0 && bin < num_bins) {
            histogram[bin]++;
        }
    }
    metrics.histogram = histogram;

    // TRIANGULAR INDEX i TiNN
    auto max_histogram_it = std::max_element(histogram.begin(), histogram.end());
    double max_bin_height = *max_histogram_it;
    if (max_bin_height > 0.0) {
        metrics.triangular_index = static_cast<double>(rr_intervals.size()) / max_bin_height;
        metrics.tinn = static_cast<double>(rr_intervals.size()) / (max_bin_height * bin_width);
    }

    // ANALIZA POINCARE (SD1, SD2)
    if (rr_intervals.size() > 1) {
        std::vector<double> x_values;
        std::vector<double> y_values;
        x_values.reserve(rr_intervals.size() - 1);
        y_values.reserve(rr_intervals.size() - 1);

        for (size_t i = 1; i < rr_intervals.size(); ++i) {
            double rr_curr = rr_intervals[i];
            double rr_prev = rr_intervals[i - 1];
            double x_value = (rr_curr + rr_prev) / std::sqrt(2.0);
            double y_value = (rr_curr - rr_prev) / std::sqrt(2.0);
            x_values.push_back(x_value);
            y_values.push_back(y_value);
        }

        double mean_x = std::accumulate(x_values.begin(), x_values.end(), 0.0) / x_values.size();
        double mean_y = std::accumulate(y_values.begin(), y_values.end(), 0.0) / y_values.size();

        double var_x_sum = 0.0;
        double var_y_sum = 0.0;
        for (size_t i = 0; i < x_values.size(); ++i) {
            double dx = x_values[i] - mean_x;
            double dy = y_values[i] - mean_y;
            var_x_sum += dx * dx;
            var_y_sum += dy * dy;
        }

        if (x_values.size() > 1) {
            metrics.sd2 = std::sqrt(var_x_sum / (static_cast<double>(x_values.size()) - 1.0));
            metrics.sd1 = std::sqrt(var_y_sum / (static_cast<double>(y_values.size()) - 1.0));
        } else {
            metrics.sd1 = 0.0;
            metrics.sd2 = 0.0;
        }
    }

    return metrics;
}