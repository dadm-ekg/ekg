#ifndef EKG_HRV_TIME_PROCESSING_SERVICE_H
#define EKG_HRV_TIME_PROCESSING_SERVICE_H
#include <vector>

#include "../../dto/hrv_time_metrics.h"
#include "../../model/r_peaks_annotated_signal_datapoint.h"
#include "../../model/signal_datapoint.h"

class IHRVTimeProcessingService {
public:
    virtual ~IHRVTimeProcessingService() = default;

    // Przetwarza sygnał EKG i wykryte piki R w celu obliczenia metryk czasowych i częstotliwościowych HRV
    // datapoints - przefiltrowany sygnał EKG
    // r_peaks - wykryte piki R (może być pusty wektor, wtedy metoda zwróci domyślne wartości)
    // frequency - częstotliwość próbkowania sygnału
    // method - metoda estymacji widma (Classic Periodogram, Lomb-Scargle, Welch)
    virtual HRVTimeMetrics Process(
        const std::vector<SignalDatapoint>& datapoints,
        const std::vector<RPeaksAnnotatedSignalDatapoint>& r_peaks,
        int frequency,
        HRVTimeMetrics::SpectralMethod method = HRVTimeMetrics::SpectralMethod::CLASSIC_PERIODOGRAM
    ) = 0;
};

#endif //EKG_HRV_TIME_PROCESSING_SERVICE_H

