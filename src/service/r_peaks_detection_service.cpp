#include "../../include/service/r_peaks_detection_service.h"
#include <vector>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace {
    // ---------------------------------------------------------------
    // ENUM
    // ---------------------------------------------------------------

    const char *MethodName(RPeaksDetectionMethod m) {
        switch (m) {
            case RPeaksDetectionMethod::PanTompkins: return "Pan-Tompkins";
            case RPeaksDetectionMethod::Hilbert: return "Hilbert";
            case RPeaksDetectionMethod::Wavelet: return "Wavelet";
            default: return "Unknown";
        }
    }

    // ---------------------------------------------------------------
    // Pomocniczy percentyl
    // ---------------------------------------------------------------
    float Percentile(const std::vector<float> &data, float p) {
        if (data.empty()) return 0.0f;
        if (p <= 0.0f) return *std::min_element(data.begin(), data.end());
        if (p >= 1.0f) return *std::max_element(data.begin(), data.end());

        std::vector<float> tmp = data;
        size_t idx = static_cast<size_t>(p * (tmp.size() - 1));
        std::nth_element(tmp.begin(), tmp.begin() + idx, tmp.end());
        return tmp[idx];
    }

    // ===============================================================
    // ====================== PAN–TOMPKINS ===========================
    // ===============================================================
    std::vector<int> DetectPeaksPanTompkins(const std::vector<float> &signal, int frequency) {
        std::vector<int> peaks;
        if (signal.size() < 5) return peaks;

        std::vector<float> diff(signal.size());
        diff[0] = 0.0f;
        for (size_t i = 1; i < signal.size(); ++i)
            diff[i] = signal[i] - signal[i - 1];

        std::vector<float> squared(signal.size());
        for (size_t i = 0; i < signal.size(); ++i)
            squared[i] = diff[i] * diff[i];

        int window = std::max(2, frequency / 35);
        std::vector<float> energy(signal.size(), 0.0f);
        float acc = 0.0f;
        int cnt = 0;

        for (int i = 0; i < window && i < (int) signal.size(); ++i) {
            acc += squared[i];
            cnt++;
        }
        if (cnt == 0) return peaks;
        energy[0] = acc / cnt;

        for (size_t i = 1; i < signal.size(); ++i) {
            if (i + window < signal.size()) {
                acc += squared[i + window];
                cnt++;
            }
            acc -= squared[i - 1];
            cnt--;
            if (cnt <= 0) cnt = 1;
            energy[i] = acc / cnt;
        }

        float thr = Percentile(energy, 0.80f);
        if (thr <= 0.0f)
            thr = 0.3f * (*std::max_element(energy.begin(), energy.end()));

        int refractory = frequency / 5;
        int last = -refractory;

        for (size_t i = 1; i + 1 < energy.size(); ++i) {
            if (energy[i] >= thr &&
                energy[i] >= energy[i - 1] &&
                energy[i] >= energy[i + 1] &&
                (int) i - last >= refractory) {
                peaks.push_back((int) i);
                last = (int) i;
            }
        }

        return peaks;
    }

    // ===============================================================
    // ======================= HILBERT ===============================
    // ===============================================================
    std::vector<int> DetectPeaksHilbert(const std::vector<float> &signal, int frequency) {
        std::vector<int> peaks;
        if (signal.size() < 20) return peaks;

        int order = frequency / 10;
        if (order < 5) order = 5;
        if (order > (int) signal.size() / 2) order = signal.size() / 2;

        std::vector<float> hil(signal.size(), 0.0f);
        for (size_t n = 0; n < signal.size(); ++n) {
            float sum = 0.0f;
            for (int k = -order; k <= order; ++k) {
                int idx = (int) n + k;
                if (idx >= 0 && idx < (int) signal.size() && k % 2 != 0) {
                    float kernel = 2.0f / (3.14159265359f * k);
                    sum += signal[idx] * kernel;
                }
            }
            hil[n] = sum;
        }

        std::vector<float> envelope(signal.size());
        for (size_t i = 0; i < signal.size(); ++i)
            envelope[i] = std::sqrt(signal[i] * signal[i] + hil[i] * hil[i]);

        int win = std::max(2, frequency / 25);
        std::vector<float> smoothed(signal.size());
        float acc = 0.0f;
        int cnt = 0;

        for (int i = 0; i < win && i < (int) envelope.size(); ++i) {
            acc += envelope[i];
            cnt++;
        }
        if (cnt == 0) return peaks;
        smoothed[0] = acc / cnt;

        for (size_t i = 1; i < envelope.size(); ++i) {
            if (i + win < envelope.size()) {
                acc += envelope[i + win];
                cnt++;
            }
            acc -= envelope[i - 1];
            cnt--;
            if (cnt <= 0) cnt = 1;
            smoothed[i] = acc / cnt;
        }

        float thr = Percentile(smoothed, 0.80f);
        if (thr <= 0.0f)
            thr = 0.3f * (*std::max_element(smoothed.begin(), smoothed.end()));

        int refractory = frequency / 5;
        int last = -refractory;

        for (size_t i = 1; i + 1 < smoothed.size(); ++i) {
            if (smoothed[i] >= thr &&
                smoothed[i] >= smoothed[i - 1] &&
                smoothed[i] >= smoothed[i + 1] &&
                (int) i - last >= refractory) {
                peaks.push_back((int) i);
                last = (int) i;
            }
        }

        return peaks;
    }

    // ===============================================================
    // ========================= WAVELET =============================
    // ===============================================================
    std::vector<int> DetectPeaksWavelet(const std::vector<float> &signal, int frequency) {
        std::vector<int> peaks;
        if (signal.size() < 20) return peaks;

        int smin = 2;
        int smax = std::max(smin + 1, frequency / 25);

        std::vector<float> response(signal.size(), 0.0f);

        for (int s = smin; s <= smax; ++s) {
            for (size_t i = s; i + s < signal.size(); ++i) {
                float d2 = signal[i + s] - 2.0f * signal[i] + signal[i - s];
                response[i] += (d2 * d2) / (float) (s * s);
            }
        }

        float mx = *std::max_element(response.begin(), response.end());
        if (mx > 0.0f)
            for (auto &v: response) v /= mx;

        int win = std::max(2, frequency / 40);
        std::vector<float> smoothed(response.size(), 0.0f);
        float acc = 0.0f;
        int cnt = 0;

        for (int i = 0; i < win && i < (int) response.size(); ++i) {
            acc += response[i];
            cnt++;
        }
        if (cnt == 0) return peaks;
        smoothed[0] = acc / cnt;

        for (size_t i = 1; i < response.size(); ++i) {
            if (i + win < response.size()) {
                acc += response[i + win];
                cnt++;
            }
            acc -= response[i - 1];
            cnt--;
            if (cnt <= 0) cnt = 1;
            smoothed[i] = acc / cnt;
        }

        float thr = Percentile(smoothed, 0.80f);
        if (thr <= 0.0f)
            thr = 0.3f * (*std::max_element(smoothed.begin(), smoothed.end()));

        int refractory = frequency / 5;
        int last = -refractory;

        for (size_t i = 1; i + 1 < smoothed.size(); ++i) {
            if (smoothed[i] >= thr &&
                smoothed[i] >= smoothed[i - 1] &&
                smoothed[i] >= smoothed[i + 1] &&
                (int) i - last >= refractory) {
                peaks.push_back((int) i);
                last = (int) i;
            }
        }

        return peaks;
    }

    // ===============================================================
    // ====================== METRYKI / RAPORT =======================
    // ===============================================================
    struct DetectionMetrics {
        int total_peaks;
        double mean_amplitude;
        double std_amplitude;
        RPeaksDetectionMethod method;
    };

    DetectionMetrics ComputeMetrics(const std::vector<int> &peak_indices,
                                    const std::vector<float> &signal,
                                    RPeaksDetectionMethod method) {
        DetectionMetrics m;
        m.method = method;
        m.total_peaks = (int) peak_indices.size();

        if (peak_indices.empty()) {
            m.mean_amplitude = 0.0;
            m.std_amplitude = 0.0;
            return m;
        }

        double sum = 0.0;
        for (int idx: peak_indices)
            if (idx >= 0 && idx < (int) signal.size())
                sum += signal[idx];

        m.mean_amplitude = sum / peak_indices.size();

        double s2 = 0.0;
        for (int idx: peak_indices) {
            if (idx >= 0 && idx < (int) signal.size()) {
                double d = signal[idx] - m.mean_amplitude;
                s2 += d * d;
            }
        }
        m.std_amplitude = std::sqrt(s2 / peak_indices.size());

        return m;
    }

    void PrintComparisonReport(const std::vector<DetectionMetrics> &all_metrics,
                               int frequency) {
        std::cout << "\n========== RAPORT PORÓWNANIA DETEKCJI PIKÓW R ==========\n";
        std::cout << "Częstotliwość próbkowania: " << frequency << " Hz\n";
        std::cout << "=======================================================\n";

        for (const auto &m: all_metrics) {
            std::cout << "\nMetoda: " << MethodName(m.method) << "\n";
            std::cout << "  Wykryte piki R: " << m.total_peaks << "\n";
            std::cout << "  Średnia amplituda: " << m.mean_amplitude << "\n";
            std::cout << "  Odchylenie standardowe: " << m.std_amplitude << "\n";
        }

        std::cout << "=======================================================\n";
    }
} // namespace

// ===============================================================
// ========================= DETECT ===============================
// ===============================================================
std::vector<RPeaksAnnotatedSignalDatapoint> RPeaksDetectionService::Detect(
    const std::vector<SignalDatapoint> &datapoints, int frequency,
    RPeaksDetectionMethod method) {
    if (datapoints.empty() || frequency <= 0)
        return {};

    // domyślna metoda
    if (method == RPeaksDetectionMethod::PanTompkins ||
        method == RPeaksDetectionMethod::Hilbert ||
        method == RPeaksDetectionMethod::Wavelet) {
        // OK
    } else {
        method = RPeaksDetectionMethod::PanTompkins;
    }

    std::vector<float> signal(datapoints.size());
    for (size_t i = 0; i < datapoints.size(); ++i)
        signal[i] = datapoints[i].channelValues[1];

    std::vector<int> peaks_pan = DetectPeaksPanTompkins(signal, frequency);
    std::vector<int> peaks_hil = DetectPeaksHilbert(signal, frequency);
    std::vector<int> peaks_wave = DetectPeaksWavelet(signal, frequency);

    std::vector<DetectionMetrics> metrics{
        ComputeMetrics(peaks_pan, signal, RPeaksDetectionMethod::PanTompkins),
        ComputeMetrics(peaks_hil, signal, RPeaksDetectionMethod::Hilbert),
        ComputeMetrics(peaks_wave, signal, RPeaksDetectionMethod::Wavelet)
    };

    PrintComparisonReport(metrics, frequency);

    // wybór metody
    const std::vector<int> *selected = nullptr;

    switch (method) {
        case RPeaksDetectionMethod::PanTompkins: selected = &peaks_pan;
            break;
        case RPeaksDetectionMethod::Hilbert: selected = &peaks_hil;
            break;
        case RPeaksDetectionMethod::Wavelet: selected = &peaks_wave;
            break;
    }

    const auto &final_peaks = *selected;

    std::vector<RPeaksAnnotatedSignalDatapoint> result;
    result.reserve(datapoints.size());

    for (size_t i = 0; i < datapoints.size(); ++i) {
        RPeaksAnnotatedSignalDatapoint dp;
        dp.channelValues = datapoints[i].channelValues;
        dp.peak = std::find(final_peaks.begin(), final_peaks.end(),
                            (int) i) != final_peaks.end();
        result.push_back(dp);
    }

    return result;
}
