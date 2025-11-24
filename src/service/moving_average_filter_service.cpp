#include "../../include/service/moving_average_filter_service.h"
#include <iostream>

std::vector<SignalDatapoint> MovingAverageFilterService::Filter(const std::vector<SignalDatapoint>& values) {
    std::vector<SignalDatapoint> filtered(values.size()); // tworzenie wektora o takiej samej długości jak values
    int window_size = 5;  // rozmiar okna

    if (values.empty() || window_size <= 1) // warunek brzegowy gdy wejście jest puste zwracam sygnał
        return values;

    if (values[0].channelValues.empty())
        return values;

    const size_t numChannels = values[0].channelValues.size();
    std::vector<double> sum(numChannels, 0.0); // suma wartości aktualnego okna

    for (size_t i = 0; i < values.size(); ++i) {
        filtered[i].channelValues.resize(numChannels);

        for (size_t ch = 0; ch < numChannels; ++ch) {
            sum[ch] += values[i].channelValues[ch];
            // Jeśli przekroczono długość okna to wykonanie usunięcia najstarszej próbki z sumy
            if (i >= static_cast<size_t>(window_size))
                sum[ch] -= values[i - window_size].channelValues[ch];

            // Liczba próbek w bieżącym oknie (dla początku sygnału)
            size_t current_window = (i + 1 < static_cast<size_t>(window_size)) ? (i + 1) : window_size;
            float avg = static_cast<float>(sum[ch] / current_window);

            filtered[i].channelValues[ch] = avg;
        }
    }

    std::cout << "Moving average filter finished" << std::endl;

    return filtered;
}
