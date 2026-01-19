#include "../../include/service/butterworth_filter_service.h"
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

std::vector<SignalDatapoint> ButterworthFilterService::Filter(const std::vector<SignalDatapoint>& values, int windowSize, int polynomialOrder) {
    std::vector<SignalDatapoint> filtered(values.size());

    if (values.size() < 3)
        return values;

    if (values.empty() || values[0].channelValues.empty())
        return values;

    double cutoffFreq = (windowSize > 0 && windowSize <= 200) ? static_cast<double>(windowSize) : 40.0;
    double fs = (polynomialOrder >= 100 && polynomialOrder <= 10000) ? static_cast<double>(polynomialOrder) : 500.0;

    if (cutoffFreq <= 0) cutoffFreq = 40.0;
    if (fs <= 0) fs = 500.0;

    const size_t numChannels = values[0].channelValues.size();

    double K = tan(M_PI * cutoffFreq / fs);
    double K2 = K * K;
    double norm = 1.0 / (1.0 + std::sqrt(2.0) * K + K2);

    double b0 = K2 * norm;
    double b1 = 2.0 * b0;
    double b2 = b0;
    double a1 = 2.0 * (K2 - 1.0) * norm;
    double a2 = (1.0 - std::sqrt(2.0) * K + K2) * norm;

    std::vector<double> x1(numChannels, 0.0);
    std::vector<double> x2(numChannels, 0.0);
    std::vector<double> y1(numChannels, 0.0);
    std::vector<double> y2(numChannels, 0.0);

    for (size_t i = 0; i < values.size(); ++i) {
        filtered[i].channelValues.resize(numChannels);

        for (size_t ch = 0; ch < numChannels; ++ch) {
            double x0 = values[i].channelValues[ch];

            double y0 = b0 * x0
                + b1 * x1[ch]
                + b2 * x2[ch]
                - a1 * y1[ch]
                - a2 * y2[ch];

            x2[ch] = x1[ch];
            x1[ch] = x0;
            y2[ch] = y1[ch];
            y1[ch] = y0;

            filtered[i].channelValues[ch] = static_cast<float>(y0);
        }
    }

    std::cout << "Butterworth filter finished (fc=" << cutoffFreq
        << " Hz, fs=" << fs << " Hz)" << std::endl;

    return filtered;
}
