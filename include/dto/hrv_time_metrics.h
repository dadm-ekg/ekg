#ifndef EKG_HRV_TIME_METRICS_H
#define EKG_HRV_TIME_METRICS_H

// Metryki czasowe i częstotliwościowe HRV
class HRVTimeMetrics {
public:
    // Parametry czasowe
    float rr_mean;      // Średnia wartość odstępów RR
    float sdnn;         // Standardowe odchylenie odstępów RR
    float rmssd;        // Pierwiastek ze średniej kwadratów różnic między kolejnymi odstępami RR
    
    // Parametry częstotliwościowe
    float tp;           // Total Power - całkowita moc widma
    float vlf;          // Very Low Frequency - moc w zakresie bardzo niskich częstotliwości (0.0033-0.04 Hz)
    float lf;           // Low Frequency - moc w zakresie niskich częstotliwości (0.04-0.15 Hz)
    float hf;           // High Frequency - moc w zakresie wysokich częstotliwości (0.15-0.4 Hz)
    float lf_hf;        // Stosunek LF/HF
    
    // Metoda estymacji widma użyta do obliczeń
    enum class SpectralMethod {
        CLASSIC_PERIODOGRAM,
        LOMB_SCARGLE,
        WELCH
    };
    
    SpectralMethod method;
};

#endif //EKG_HRV_TIME_METRICS_H
