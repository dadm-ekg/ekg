#include "../../include/service/hrv_dfa_processing_service.h"
#include <vector>
#include <cmath>
#include <numeric>

namespace {

class HRVDFA
{
public:
    struct Result {
        double alpha1 = 0.0;
        double alpha2 = 0.0;
    };

    Result compute(const std::vector<double>& rr)
    {
        Result r;
        if (rr.size() < 32)
            return r;

        std::vector<double> y = integrate(rr);

        std::vector<int> scales;
        std::vector<double> fluct;

        for (int n = 4; n <= 64; ++n)
        {
            scales.push_back(n);
            fluct.push_back(F(y, n));
        }

        r.alpha1 = slope(scales, fluct, 4, 16);
        r.alpha2 = slope(scales, fluct, 16, 64);

        return r;
    }

private:
    std::vector<double> integrate(const std::vector<double>& rr)
    {
        double mean = std::accumulate(rr.begin(), rr.end(), 0.0) / rr.size();
        std::vector<double> y(rr.size());
        double sum = 0.0;
        for (size_t i = 0; i < rr.size(); ++i)
        {
            sum += rr[i] - mean;
            y[i] = sum;
        }
        return y;
    }

    double F(const std::vector<double>& y, int scale)
    {
        int segments = static_cast<int>(y.size()) / scale;
        double rms = 0.0;

        for (int v = 0; v < segments; ++v)
        {
            int start = v * scale;
            double sx = 0, sy = 0, sxx = 0, sxy = 0;

            for (int i = 0; i < scale; ++i)
            {
                double x = static_cast<double>(i);
                double val = y[static_cast<size_t>(start + i)];
                sx += x;
                sy += val;
                sxx += x * x;
                sxy += x * val;
            }

            double a = (scale * sxy - sx * sy) /
                       (scale * sxx - sx * sx);
            double b = (sy - a * sx) / scale;

            for (int i = 0; i < scale; ++i)
            {
                double trend = a * i + b;
                double diff = y[static_cast<size_t>(start + i)] - trend;
                rms += diff * diff;
            }
        }

        return std::sqrt(rms / (segments * scale));
    }

    double slope(
        const std::vector<int>& scales,
        const std::vector<double>& fluct,
        int minN, int maxN)
    {
        double mx = 0, my = 0;
        int count = 0;

        for (size_t i = 0; i < scales.size(); ++i)
        {
            if (scales[i] >= minN && scales[i] <= maxN)
            {
                mx += std::log(scales[i]);
                my += std::log(fluct[i]);
                count++;
            }
        }

        if (count == 0) return 0.0;

        mx /= count;
        my /= count;

        double num = 0, den = 0;
        for (size_t i = 0; i < scales.size(); ++i)
        {
            if (scales[i] >= minN && scales[i] <= maxN)
            {
                double x = std::log(scales[i]) - mx;
                double y = std::log(fluct[i]) - my;
                num += x * y;
                den += x * x;
            }
        }

        return (std::abs(den) < 1e-12) ? 0.0 : (num / den);
    }
};

std::vector<double> extractRRFromRPeaks(
    const std::vector<RPeaksAnnotatedSignalDatapoint>& datapoints,
    int frequency)
{
    std::vector<size_t> peak_indices;
    for (size_t i = 0; i < datapoints.size(); ++i)
    {
        if (!datapoints[i].peaks.empty() && datapoints[i].peaks[0])
            peak_indices.push_back(i);
    }

    std::vector<double> rr;
    for (size_t i = 1; i < peak_indices.size(); ++i)
    {
        double interval_ms = (peak_indices[i] - peak_indices[i - 1]) * 1000.0 / frequency;
        if (interval_ms > 300.0 && interval_ms < 2000.0)
            rr.push_back(interval_ms);
    }
    return rr;
}

}

HRVDFAMetrics HRVDFAProcessingService::Process(
    const std::vector<RPeaksAnnotatedSignalDatapoint>& datapoints,
    int frequency)
{
    HRVDFAMetrics metrics{};

    std::vector<double> rr = extractRRFromRPeaks(datapoints, frequency);

    HRVDFA dfa;
    auto res = dfa.compute(rr);

    metrics.alpha1 = static_cast<float>(res.alpha1);
    metrics.alpha2 = static_cast<float>(res.alpha2);

    return metrics;
}
