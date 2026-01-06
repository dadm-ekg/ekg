#include "../../include/service/moving_average_filter_service.h"
#include <iostream>

std::vector<SignalDatapoint> MovingAverageFilterService::Filter(const std::vector<SignalDatapoint>& values, int windowSize, int) {
    std::vector<SignalDatapoint> filtered(values.size());
    
    if (windowSize < 1) windowSize = 5;

    if (values.empty() || windowSize <= 1)
        return values;

    if (values[0].channelValues.empty())
        return values;

    const size_t numChannels = values[0].channelValues.size();
    std::vector<double> sum(numChannels, 0.0);

    for (size_t i = 0; i < values.size(); ++i) {
        filtered[i].channelValues.resize(numChannels);

        for (size_t ch = 0; ch < numChannels; ++ch) {
            sum[ch] += values[i].channelValues[ch];
            if (i >= static_cast<size_t>(windowSize))
                sum[ch] -= values[i - windowSize].channelValues[ch];

            size_t current_window = (i + 1 < static_cast<size_t>(windowSize)) ? (i + 1) : windowSize;
            float avg = static_cast<float>(sum[ch] / current_window);

            filtered[i].channelValues[ch] = avg;
        }
    }

    std::cout << "Moving average filter finished (window=" << windowSize << ")" << std::endl;

    return filtered;
}
