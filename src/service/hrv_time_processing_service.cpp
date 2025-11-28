#include "../../include/service/hrv_time_processing_service.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <numeric>
#include <complex>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Pomocnicza funkcja do ekstrakcji odstępów RR z wykrytych pików R
std::vector<double> ExtractRRIntervals(
    const std::vector<RPeaksAnnotatedSignalDatapoint>& r_peaks,
    int frequency) {
    std::vector<double> rr_intervals;
    
    if (r_peaks.empty()) {
        return rr_intervals;
    }
    
    // Znajdź wszystkie indeksy, gdzie peak == true
    std::vector<size_t> peak_indices;
    for (size_t i = 0; i < r_peaks.size(); ++i) {
        if (r_peaks[i].peak) {
            peak_indices.push_back(i);
        }
    }
    
    // Oblicz odstępy RR w milisekundach
    for (size_t i = 1; i < peak_indices.size(); ++i) {
        double interval_ms = (peak_indices[i] - peak_indices[i - 1]) * 1000.0 / frequency;
        if (interval_ms > 300.0 && interval_ms < 2000.0) { // Filtrowanie artefaktów
            rr_intervals.push_back(interval_ms);
        }
    }
    
    return rr_intervals;
}

// Oblicz parametry czasowe HRV
void CalculateTimeDomainMetrics(
    const std::vector<double>& rr_intervals,
    HRVTimeMetrics& metrics) {
    
    if (rr_intervals.empty()) {
        metrics.rr_mean = 0.0f;
        metrics.sdnn = 0.0f;
        metrics.rmssd = 0.0f;
        return;
    }
    
    // RR_mean - średnia wartość odstępów RR
    double sum = std::accumulate(rr_intervals.begin(), rr_intervals.end(), 0.0);
    metrics.rr_mean = static_cast<float>(sum / rr_intervals.size());
    
    // SDNN - standardowe odchylenie odstępów RR
    double variance = 0.0;
    for (double rr : rr_intervals) {
        double diff = rr - metrics.rr_mean;
        variance += diff * diff;
    }
    variance /= rr_intervals.size();
    metrics.sdnn = static_cast<float>(std::sqrt(variance));
    
    // RMSSD - pierwiastek ze średniej kwadratów różnic między kolejnymi odstępami RR
    if (rr_intervals.size() > 1) {
        double sum_squared_diff = 0.0;
        for (size_t i = 1; i < rr_intervals.size(); ++i) {
            double diff = rr_intervals[i] - rr_intervals[i - 1];
            sum_squared_diff += diff * diff;
        }
        metrics.rmssd = static_cast<float>(std::sqrt(sum_squared_diff / (rr_intervals.size() - 1)));
    } else {
        metrics.rmssd = 0.0f;
    }
}

// Interpolacja sygnału RR do równomiernego próbkowania
std::vector<double> InterpolateRRSignal(
    const std::vector<double>& rr_intervals,
    double target_sampling_rate = 4.0) { // 4 Hz - standardowa częstotliwość dla HRV
    
    if (rr_intervals.empty()) {
        return std::vector<double>();
    }
    
    // Oblicz skumulowane czasy dla każdego odstępu RR
    std::vector<double> cumulative_times;
    cumulative_times.push_back(0.0);
    for (size_t i = 0; i < rr_intervals.size(); ++i) {
        cumulative_times.push_back(cumulative_times.back() + rr_intervals[i]);
    }
    
    // Interpolacja liniowa
    double total_time = cumulative_times.back();
    size_t num_samples = static_cast<size_t>(total_time * target_sampling_rate / 1000.0);
    
    std::vector<double> interpolated(num_samples);
    double dt = 1000.0 / target_sampling_rate; // krok czasowy w ms
    
    for (size_t i = 0; i < num_samples; ++i) {
        double t = i * dt;
        
        size_t interval_idx = 0;
        for (size_t j = 0; j < cumulative_times.size() - 1; ++j) {
            if (t >= cumulative_times[j] && t < cumulative_times[j + 1]) {
                interval_idx = j;
                break;
            }
        }
        
        if (interval_idx < rr_intervals.size()) {
            interpolated[i] = rr_intervals[interval_idx];
        } else {
            interpolated[i] = rr_intervals.back();
        }
    }
    
    return interpolated;
}

// Klasyczny periodogram (FFT)
std::vector<double> ClassicPeriodogram(const std::vector<double>& signal, double sampling_rate) {
    size_t N = signal.size();
    if (N == 0) {
        return std::vector<double>();
    }
    
    // Oblicz średnią i usuń składową stałą
    double mean = std::accumulate(signal.begin(), signal.end(), 0.0) / N;
    std::vector<double> centered_signal(N);
    for (size_t i = 0; i < N; ++i) {
        centered_signal[i] = signal[i] - mean;
    }
    
    // Prosta implementacja DFT (dla małych sygnałów)
    // Dla większych sygnałów warto użyć FFT (np. FFTW)
    size_t fft_size = N;
    std::vector<double> power_spectrum(fft_size / 2 + 1);
    
    for (size_t k = 0; k <= fft_size / 2; ++k) {
        std::complex<double> sum(0.0, 0.0);
        for (size_t n = 0; n < N; ++n) {
            double angle = -2.0 * M_PI * k * n / fft_size;
            sum += centered_signal[n] * std::complex<double>(std::cos(angle), std::sin(angle));
        }
        power_spectrum[k] = std::norm(sum) / (N * N);
    }
    
    return power_spectrum;
}

// Periodogram Lomb-Scargle
std::vector<double> LombScarglePeriodogram(
    const std::vector<double>& signal,
    const std::vector<double>& times,
    const std::vector<double>& frequencies) {
    
    if (signal.size() != times.size() || signal.empty()) {
        return std::vector<double>();
    }
    
    std::vector<double> power_spectrum(frequencies.size());
    
    // Oblicz średnią i wariancję
    double mean = std::accumulate(signal.begin(), signal.end(), 0.0) / signal.size();
    double variance = 0.0;
    for (double val : signal) {
        double diff = val - mean;
        variance += diff * diff;
    }
    variance /= signal.size();
    
    if (variance < 1e-10) {
        return std::vector<double>(frequencies.size(), 0.0);
    }
    
    for (size_t i = 0; i < frequencies.size(); ++i) {
        double omega = 2.0 * M_PI * frequencies[i];
        
        // Oblicz tau (optymalne przesunięcie fazowe)
        double sin_sum = 0.0, cos_sum = 0.0;
        for (size_t j = 0; j < times.size(); ++j) {
            double angle = omega * times[j];
            sin_sum += std::sin(2.0 * angle);
            cos_sum += std::cos(2.0 * angle);
        }
        double tau = std::atan2(sin_sum, cos_sum) / (2.0 * omega);
        
        // Oblicz periodogram
        double cos_sum_val = 0.0, sin_sum_val = 0.0;
        double cos_norm = 0.0, sin_norm = 0.0;
        
        for (size_t j = 0; j < times.size(); ++j) {
            double angle = omega * (times[j] - tau);
            double cos_val = std::cos(angle);
            double sin_val = std::sin(angle);
            double centered_val = signal[j] - mean;
            
            cos_sum_val += centered_val * cos_val;
            sin_sum_val += centered_val * sin_val;
            cos_norm += cos_val * cos_val;
            sin_norm += sin_val * sin_val;
        }
        
        double power = (cos_sum_val * cos_sum_val / cos_norm + 
                       sin_sum_val * sin_sum_val / sin_norm) / (2.0 * variance);
        power_spectrum[i] = power;
    }
    
    return power_spectrum;
}

// Periodogram Welch (z oknem i nakładaniem)
std::vector<double> WelchPeriodogram(
    const std::vector<double>& signal,
    double sampling_rate,
    size_t window_size = 256,
    double overlap = 0.5) {
    
    if (signal.empty()) {
        return std::vector<double>();
    }
    
    size_t N = signal.size();
    size_t step = static_cast<size_t>(window_size * (1.0 - overlap));
    size_t num_segments = (N - window_size) / step + 1;
    
    if (num_segments == 0) {
        return ClassicPeriodogram(signal, sampling_rate);
    }
    
    // Oblicz średnią i usuń składową stałą
    double mean = std::accumulate(signal.begin(), signal.end(), 0.0) / N;
    std::vector<double> centered_signal(N);
    for (size_t i = 0; i < N; ++i) {
        centered_signal[i] = signal[i] - mean;
    }
    
    // Okno Hann
    std::vector<double> window(window_size);
    double window_sum = 0.0;
    for (size_t i = 0; i < window_size; ++i) {
        window[i] = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (window_size - 1)));
        window_sum += window[i] * window[i];
    }
    
    size_t fft_size = window_size;
    std::vector<double> power_spectrum_sum(fft_size / 2 + 1, 0.0);
    
    // Przetwarzaj każdy segment
    for (size_t seg = 0; seg < num_segments; ++seg) {
        size_t start = seg * step;
        if (start + window_size > N) break;
        
        // Zastosuj okno i oblicz DFT
        std::vector<double> windowed(window_size);
        for (size_t i = 0; i < window_size; ++i) {
            windowed[i] = centered_signal[start + i] * window[i];
        }
        
        for (size_t k = 0; k <= fft_size / 2; ++k) {
            std::complex<double> sum(0.0, 0.0);
            for (size_t n = 0; n < window_size; ++n) {
                double angle = -2.0 * M_PI * k * n / fft_size;
                sum += windowed[n] * std::complex<double>(std::cos(angle), std::sin(angle));
            }
            power_spectrum_sum[k] += std::norm(sum);
        }
    }
    
    // Uśrednij i znormalizuj
    std::vector<double> power_spectrum(fft_size / 2 + 1);
    double normalization = window_sum * num_segments * fft_size;
    for (size_t k = 0; k <= fft_size / 2; ++k) {
        power_spectrum[k] = power_spectrum_sum[k] / normalization;
    }
    
    return power_spectrum;
}

// Oblicz parametry częstotliwościowe z widma mocy
void CalculateFrequencyDomainMetrics(
    const std::vector<double>& power_spectrum,
    const std::vector<double>& frequencies,
    double sampling_rate,
    HRVTimeMetrics& metrics) {
    
    if (power_spectrum.empty() || frequencies.empty()) {
        metrics.tp = 0.0f;
        metrics.vlf = 0.0f;
        metrics.lf = 0.0f;
        metrics.hf = 0.0f;
        metrics.lf_hf = 0.0f;
        return;
    }
    
    // Zakresy częstotliwości dla HRV (w Hz)
    const double VLF_MIN = 0.0033;
    const double VLF_MAX = 0.04;
    const double LF_MIN = 0.04;
    const double LF_MAX = 0.15;
    const double HF_MIN = 0.15;
    const double HF_MAX = 0.4;
    
    double df = frequencies.size() > 1 ? (frequencies[1] - frequencies[0]) : (sampling_rate / 2.0 / power_spectrum.size());
    
    double tp = 0.0, vlf = 0.0, lf = 0.0, hf = 0.0;
    
    for (size_t i = 0; i < power_spectrum.size() && i < frequencies.size(); ++i) {
        double freq = frequencies[i];
        double power = power_spectrum[i];
        
        tp += power * df;
        
        if (freq >= VLF_MIN && freq < VLF_MAX) {
            vlf += power * df;
        } else if (freq >= LF_MIN && freq < LF_MAX) {
            lf += power * df;
        } else if (freq >= HF_MIN && freq <= HF_MAX) {
            hf += power * df;
        }
    }
    
    metrics.tp = static_cast<float>(tp);
    metrics.vlf = static_cast<float>(vlf);
    metrics.lf = static_cast<float>(lf);
    metrics.hf = static_cast<float>(hf);
    metrics.lf_hf = (metrics.hf > 1e-10) ? (metrics.lf / metrics.hf) : 0.0f;
}

HRVTimeMetrics HRVTimeProcessingService::Process(
    const std::vector<SignalDatapoint>& datapoints,
    const std::vector<RPeaksAnnotatedSignalDatapoint>& r_peaks,
    int frequency,
    HRVTimeMetrics::SpectralMethod method) {
    
    HRVTimeMetrics metrics;
    metrics.method = method;
    
    // Jeśli nie ma wykrytych pików R, zwróć domyślne wartości
    if (r_peaks.empty()) {
        metrics.rr_mean = 0.0f;
        metrics.sdnn = 0.0f;
        metrics.rmssd = 0.0f;
        metrics.tp = 0.0f;
        metrics.vlf = 0.0f;
        metrics.lf = 0.0f;
        metrics.hf = 0.0f;
        metrics.lf_hf = 0.0f;
        return metrics;
    }
    
    // 1. Ekstrakcja odstępów RR
    std::vector<double> rr_intervals = ExtractRRIntervals(r_peaks, frequency);
    
    if (rr_intervals.empty()) {
        metrics.rr_mean = 0.0f;
        metrics.sdnn = 0.0f;
        metrics.rmssd = 0.0f;
        metrics.tp = 0.0f;
        metrics.vlf = 0.0f;
        metrics.lf = 0.0f;
        metrics.hf = 0.0f;
        metrics.lf_hf = 0.0f;
        return metrics;
    }
    
    // 2. Oblicz parametry czasowe
    CalculateTimeDomainMetrics(rr_intervals, metrics);
    
    // 3. Interpolacja sygnału RR
    double target_sampling_rate = 4.0; // 4 Hz - standardowa częstotliwość dla HRV
    std::vector<double> interpolated_rr = InterpolateRRSignal(rr_intervals, target_sampling_rate);
    
    if (interpolated_rr.empty()) {
        metrics.tp = 0.0f;
        metrics.vlf = 0.0f;
        metrics.lf = 0.0f;
        metrics.hf = 0.0f;
        metrics.lf_hf = 0.0f;
        return metrics;
    }
    
    // 4. Oblicz periodogram w zależności od wybranej metody
    std::vector<double> power_spectrum;
    std::vector<double> frequencies;
    
    if (method == HRVTimeMetrics::SpectralMethod::CLASSIC_PERIODOGRAM) {
        power_spectrum = ClassicPeriodogram(interpolated_rr, target_sampling_rate);
        // Generuj wektor częstotliwości
        frequencies.resize(power_spectrum.size());
        for (size_t i = 0; i < frequencies.size(); ++i) {
            frequencies[i] = i * target_sampling_rate / (2.0 * power_spectrum.size());
        }
    } else if (method == HRVTimeMetrics::SpectralMethod::LOMB_SCARGLE) {
        // Dla Lomb-Scargle potrzebujemy nieregularnych czasów
        std::vector<double> times;
        double cumulative_time = 0.0;
        for (size_t i = 0; i < rr_intervals.size(); ++i) {
            times.push_back(cumulative_time);
            cumulative_time += rr_intervals[i];
        }
        
        // Generuj częstotliwości do analizy (0.0033 - 0.4 Hz)
        size_t num_freqs = 200;
        frequencies.resize(num_freqs);
        for (size_t i = 0; i < num_freqs; ++i) {
            frequencies[i] = 0.0033 + (0.4 - 0.0033) * i / (num_freqs - 1);
        }
        
        power_spectrum = LombScarglePeriodogram(rr_intervals, times, frequencies);
    } else if (method == HRVTimeMetrics::SpectralMethod::WELCH) {
        power_spectrum = WelchPeriodogram(interpolated_rr, target_sampling_rate);
        // Generuj wektor częstotliwości
        frequencies.resize(power_spectrum.size());
        for (size_t i = 0; i < frequencies.size(); ++i) {
            frequencies[i] = i * target_sampling_rate / (2.0 * power_spectrum.size());
        }
    }
    
    // 5. Oblicz parametry częstotliwościowe
    CalculateFrequencyDomainMetrics(power_spectrum, frequencies, target_sampling_rate, metrics);
    
    return metrics;
}
