#include "../../include/service/moving_average_filter_service.h"

std::vector<SignalDatapoint> MovingAverageFilterService::Filter(const std::vector<SignalDatapoint>& values) {
    std::vector<SignalDatapoint> filtered(values.size()); // tworzenie wektora o takiej samej długości jak values
    int window_size = 5;  // rozmiar okna

    if (values.empty() || window_size <= 1) // warunek brzegowy gdy wejście jest puste zwracam sygnał
        return values;

    double sum = 0.0; // suma wartości aktualnego okna

    for (size_t i = 0; i < values.size(); ++i) {
        sum += values[i].value;
        // Jeśli przekroczono długość okna to wykonanie usunięcia najstarszej próbki z sumy
        if (i >= window_size)
            sum -= values[i - window_size].value;

        // Liczba próbek w bieżącym oknie (dla początku sygnału)
        size_t current_window = (i + 1 < window_size) ? (i + 1) : window_size;
        float avg = static_cast<float>(sum / current_window);

        filtered[i] = values[i];
        filtered[i].value = avg;
    }

    return filtered;
    // return std::vector<SignalDatapoint>{};
}
