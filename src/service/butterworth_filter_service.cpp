#include "../../include/service/butterworth_filter_service.h"
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

std::vector<SignalDatapoint> ButterworthFilterService::Filter(const std::vector<SignalDatapoint>& values) {
    std::vector<SignalDatapoint> filtered(values.size()); // tworzenie nowego wektora filtered- nowy wynik

    if (values.size() < 3) // Jeśli sygnał ma mniej niż 3 próbki nie da się zastosować filtru 2 rzędu
        return values;  // Zwrot wartości

    // Parametry filtru Butterwortha
    double fs = 500.0; //częstotliwość próbkowania
    double fc = 40.0; //częstotliwość odcięcia
    double K = tan(M_PI * fc / fs); //przekszt. bilinearne: zamiana filtru analogowego na dyskretny
    double K2 = K * K;
    double norm = 1.0 / (1.0 + std::sqrt(2.0) * K + K2); //współczynnik normalizujący amplitudę

    // Oblicz współczynniki
    double b0 = K2 * norm;
    double b1 = 2.0 * b0;
    double b2 = b0;
    double a1 = 2.0 * (K2 - 1.0) * norm;
    double a2 = (1.0 - std::sqrt(2.0) * K + K2) * norm;


    // Równanie dla filtru IIR: y[n]=b0?x[n]+b1?x[n?1]+b2?x[n?2]?a1?y[n?1]?a2?y[n?2], gdzie x[n] to aktualny sygnał
    double x1 = 0, x2 = 0, y1 = 0, y2 = 0; // wartości startwe, x1- wej. z poprzedniego kroku, y1- poprzednia wart. wyj

    for (size_t i = 0; i < values.size(); ++i) {
        double x0 = values[i].value;
        double y0 = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

        x2 = x1;
        x1 = x0;
        y2 = y1;
        y1 = y0;

        filtered[i] = values[i];
        filtered[i].value = static_cast<float>(y0);
    }

    return filtered;
}
// return std::vector<SignalDatapoint>{};

