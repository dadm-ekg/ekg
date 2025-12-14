#include "../../include/service/r_peaks_detection_service.h"
#include <vector>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <unordered_set>

namespace {

// ==================== NARZĘDZIA (WSPÓLNE) ====================

static inline int MsToSamples(int ms, int fs) {
    return std::max(1, (ms * fs) / 1000);
}

// Prosta średnia krocząca z paddingiem "edge" (jak w Pythonie)
std::vector<float> MovingAverageEdge(const std::vector<float>& x, int w) {
    const int N = static_cast<int>(x.size());
    w = std::max(1, w);
    if (N == 0) return {};

    // prefix sum
    std::vector<double> c(N + 1, 0.0);
    for (int i = 0; i < N; ++i) c[i + 1] = c[i] + x[i];

    std::vector<float> y(N, 0.0f);
    // liczymy "valid" przez zmienne okna przy brzegach jak w Pythonie (edge padding efekt)
    // najprościej: dla każdego i bierz [i-w/2, i+w/2]
    int half = w / 2;
    for (int i = 0; i < N; ++i) {
        int a = std::max(0, i - half);
        int b = std::min(N, i + half + (w % 2 ? 1 : 0));
        if (b <= a) { y[i] = x[i]; continue; }
        y[i] = static_cast<float>((c[b] - c[a]) / (b - a));
    }
    return y;
}

// Percentyl (0-100) przez nth_element (robust, szybko)
float Percentile(std::vector<float> v, int p) {
    if (v.empty()) return 0.0f;
    p = std::clamp(p, 0, 100);
    const size_t n = v.size();
    size_t k = static_cast<size_t>(std::llround((p / 100.0) * (n - 1)));
    std::nth_element(v.begin(), v.begin() + k, v.end());
    return v[k];
}

std::vector<int> LocalMaxima(const std::vector<float>& s) {
    std::vector<int> idx;
    const int N = static_cast<int>(s.size());
    if (N < 3) return idx;
    idx.reserve(N / 10);
    for (int i = 1; i < N - 1; ++i) {
        if (s[i] > s[i - 1] && s[i] >= s[i + 1]) idx.push_back(i);
    }
    return idx;
}

// wspólne filtrowanie jak w Pythonie (bandpass-like)
struct PreparedSignal {
    std::vector<float> raw;
    std::vector<float> filt;
};

PreparedSignal PrepareSignal(const std::vector<float>& raw, int fs) {
    PreparedSignal out;
    out.raw = raw;

    int w_lp = MsToSamples(28, fs);   // ~28ms
    int w_hp = MsToSamples(800, fs);  // ~800ms

    auto x_lp = MovingAverageEdge(raw, w_lp);
    auto trend = MovingAverageEdge(x_lp, w_hp);

    out.filt.resize(raw.size(), 0.0f);
    for (size_t i = 0; i < raw.size(); ++i) out.filt[i] = x_lp[i] - trend[i];
    return out;
}

// Doprecyzowanie R: (1) max |x_filt| w ±120ms, (2) max raw w ±20ms
int RefineRPeak(int qrs_idx, const std::vector<float>& raw, const std::vector<float>& x_filt, int fs) {
    const int N = static_cast<int>(raw.size());
    const int win1 = MsToSamples(120, fs);
    const int win2 = MsToSamples(40, fs);

    int a = std::max(0, qrs_idx - win1);
    int b = std::min(N, qrs_idx + win1 + 1);
    if (b <= a) return qrs_idx;

    int r0 = a;
    float bestAbs = -1.0f;
    for (int i = a; i < b; ++i) {
        float v = std::fabs(x_filt[i]);
        if (v > bestAbs) { bestAbs = v; r0 = i; }
    }

    int aa = std::max(0, r0 - win2);
    int bb = std::min(N, r0 + win2 + 1);
    if (bb <= aa) return r0;

    int r = aa;
    float bestAbs = std::fabs(x_filt[aa]);
    for (int i = aa; i < bb; ++i) {
    float v = std::fabs(x_filt[i]);
    if (v > bestAbs) { bestAbs = v; r = i; }
    }

    return r;
}


// ==================== PAN–TOMPKINS (MANUAL) ====================
std::vector<int> DetectPeaksPanTompkins(const std::vector<float>& signal, int fs) {
    std::vector<int> r_peaks;
    const int N = static_cast<int>(signal.size());
    if (N < 10 || fs <= 0) return r_peaks;

    // 0) przygotowanie sygnału (jak Python)
    auto prep = PrepareSignal(signal, fs);
    const auto& raw = prep.raw;
    const auto& x = prep.filt;

    // 1) pochodna 5-punktowa
    std::vector<float> d(N, 0.0f);
    for (int n = 2; n < N - 2; ++n) {
        d[n] = (-x[n - 2] - 2.0f * x[n - 1] + 2.0f * x[n + 1] + x[n + 2]) / 8.0f;
    }

    // 2) kwadrat
    std::vector<float> sq(N, 0.0f);
    for (int i = 0; i < N; ++i) sq[i] = d[i] * d[i];

    // 3) MWI ~150ms
    int w_mwi = MsToSamples(150, fs);
    std::vector<float> mwi(N, 0.0f);
    std::vector<double> c(N + 1, 0.0);
    for (int i = 0; i < N; ++i) c[i + 1] = c[i] + sq[i];
    for (int i = 0; i < N; ++i) {
        int a = std::max(0, i - w_mwi + 1);
        int b = i + 1;
        mwi[i] = static_cast<float>((c[b] - c[a]) / (b - a));
    }

    // 4) kandydaci = lokalne maksima mwi
    auto cand = LocalMaxima(mwi);
    if (cand.empty()) return r_peaks;

    // 5) adaptacyjny próg SPKI/NPKI (init 2s)
    int refractory = MsToSamples(200, fs);
    int init_len = std::min(N, MsToSamples(2000, fs));

    std::vector<float> init_vals;
    init_vals.reserve(256);
    for (int idx : cand) {
        if (idx < init_len) init_vals.push_back(mwi[idx]);
        else break;
    }
    if (init_vals.empty()) return r_peaks;

    float SPKI = Percentile(init_vals, 95);
    float NPKI = Percentile(init_vals, 60);
    float THR1 = NPKI + 0.25f * (SPKI - NPKI);

    std::vector<int> qrs_idx;
    qrs_idx.reserve(cand.size() / 2);
    int last = -1e9;

    for (int idx : cand) {
        if (idx - last < refractory) continue;
        float peak = mwi[idx];

        if (peak > THR1) {
            qrs_idx.push_back(idx);
            last = idx;
            SPKI = 0.125f * peak + 0.875f * SPKI;
        } else {
            NPKI = 0.125f * peak + 0.875f * NPKI;
        }
        THR1 = NPKI + 0.25f * (SPKI - NPKI);
    }

    // 6) doprecyzowanie R w raw
    r_peaks.reserve(qrs_idx.size());
    for (int q : qrs_idx) {
        r_peaks.push_back(RefineRPeak(q, raw, x, fs));
    }

    std::sort(r_peaks.begin(), r_peaks.end());
    r_peaks.erase(std::unique(r_peaks.begin(), r_peaks.end()), r_peaks.end());
    return r_peaks;
}


// ==================== HILBERT (MANUAL FIR + ENVELOPE) ====================
std::vector<int> DetectPeaksHilbert(const std::vector<float>& signal, int fs) {
    std::vector<int> r_peaks;
    const int N = static_cast<int>(signal.size());
    if (N < 20 || fs <= 0) return r_peaks;

    // 0) przygotowanie sygnału
    auto prep = PrepareSignal(signal, fs);
    const auto& raw = prep.raw;
    const auto& x = prep.filt;

    // 1) pochodna 5-punktowa (QRS emphasis)
    std::vector<float> d(N, 0.0f);
    for (int n = 2; n < N - 2; ++n) {
        d[n] = (-x[n - 2] - 2.0f * x[n - 1] + 2.0f * x[n + 1] + x[n + 2]) / 8.0f;
    }

    // 2) FIR Hilberta z oknem Hamminga
    const float pi = 3.14159265358979323846f;
    int H = std::max(15, fs / 5);
    H = std::min(H, std::max(15, (N / 2) - 2));
    int L = 2 * H + 1;

    std::vector<float> hamming(L, 0.0f);
    for (int n = 0; n < L; ++n) {
        hamming[n] = 0.54f - 0.46f * std::cos(2.0f * pi * n / (L - 1));
    }

    std::vector<float> kernel(L, 0.0f);
    for (int k = -H; k <= H; ++k) {
        int idx = k + H;
        if (k == 0 || (k % 2) == 0) {
            kernel[idx] = 0.0f;
        } else {
            kernel[idx] = (2.0f / (pi * k)) * hamming[idx];
        }
    }

    std::vector<float> hilb(N, 0.0f);
    for (int n = 0; n < N; ++n) {
        double sum = 0.0;
        for (int k = -H; k <= H; ++k) {
            int i = n + k;
            if (i >= 0 && i < N) sum += static_cast<double>(d[i]) * kernel[k + H];
        }
        hilb[n] = static_cast<float>(sum);
    }

    // 3) obwiednia + MWI ~120ms
    std::vector<float> env(N, 0.0f);
    for (int i = 0; i < N; ++i) {
        env[i] = std::sqrt(d[i] * d[i] + hilb[i] * hilb[i]);
    }

    int w_env = MsToSamples(120, fs);
    std::vector<float> env_mwi(N, 0.0f);
    std::vector<double> c(N + 1, 0.0);
    for (int i = 0; i < N; ++i) c[i + 1] = c[i] + env[i];
    for (int i = 0; i < N; ++i) {
        int a = std::max(0, i - w_env + 1);
        int b = i + 1;
        env_mwi[i] = static_cast<float>((c[b] - c[a]) / (b - a));
    }

    // 4) lokalne maksima
    auto cand = LocalMaxima(env_mwi);
    if (cand.empty()) return r_peaks;

    // 5) SPKI/NPKI
    int refractory = MsToSamples(200, fs);
    int init_len = std::min(N, MsToSamples(2000, fs));

    std::vector<float> init_vals;
    init_vals.reserve(256);
    for (int idx : cand) {
        if (idx < init_len) init_vals.push_back(env_mwi[idx]);
        else break;
    }
    if (init_vals.empty()) return r_peaks;

    float SPKI = Percentile(init_vals, 95);
    float NPKI = Percentile(init_vals, 60);
    float THR1 = NPKI + 0.25f * (SPKI - NPKI);

    std::vector<int> qrs_idx;
    qrs_idx.reserve(cand.size() / 2);
    int last = -1e9;

    for (int idx : cand) {
        if (idx - last < refractory) continue;
        float peak = env_mwi[idx];

        if (peak > THR1) {
            qrs_idx.push_back(idx);
            last = idx;
            SPKI = 0.125f * peak + 0.875f * SPKI;
        } else {
            NPKI = 0.125f * peak + 0.875f * NPKI;
        }
        THR1 = NPKI + 0.25f * (SPKI - NPKI);
    }

    // 6) doprecyzowanie R w raw
    r_peaks.reserve(qrs_idx.size());
    for (int q : qrs_idx) {
        r_peaks.push_back(RefineRPeak(q, raw, x, fs));
    }

    std::sort(r_peaks.begin(), r_peaks.end());
    r_peaks.erase(std::unique(r_peaks.begin(), r_peaks.end()), r_peaks.end());
    return r_peaks;
}


// ==================== WAVELET-LIKE (LoG MULTI-SCALE) ====================
std::vector<int> DetectPeaksWavelet(const std::vector<float>& signal, int fs) {
    std::vector<int> r_peaks;
    const int N = static_cast<int>(signal.size());
    if (N < 30 || fs <= 0) return r_peaks;

    // 0) przygotowanie sygnału
    auto prep = PrepareSignal(signal, fs);
    const auto& raw = prep.raw;
    const auto& x = prep.filt;

    // lekkie wygładzenie przed drugą różnicą ~30ms
    auto xw = MovingAverageEdge(x, MsToSamples(30, fs));

    // skale ~20-100ms
    int scale_min = std::max(2, fs / 50);
    int scale_max = std::max(scale_min + 1, fs / 10);

    // 1) odpowiedź wieloskalowa LoG
    std::vector<float> resp(N, 0.0f);
    for (int s = scale_min; s <= scale_max; ++s) {
        float s2 = static_cast<float>(s * s);
        for (int i = s; i < N - s; ++i) {
            float second = xw[i + s] - 2.0f * xw[i] + xw[i - s];
            float coeff = second / s2;
            resp[i] += coeff * coeff;
        }
    }

    // 2) robust normalizacja percentylem 99
    float p99 = Percentile(resp, 99);
    std::vector<float> resp_n(N, 0.0f);
    if (p99 > 0.0f) {
        for (int i = 0; i < N; ++i) {
            float v = resp[i] / p99;
            resp_n[i] = std::clamp(v, 0.0f, 3.0f);
        }
    } else {
        resp_n = resp;
    }

    // 3) wygładzenie odpowiedzi ~90ms
    auto resp_sm = MovingAverageEdge(resp_n, MsToSamples(90, fs));

    // 4) lokalne maksima
    auto cand = LocalMaxima(resp_sm);
    if (cand.empty()) return r_peaks;

    // 5) SPKI/NPKI
    int refractory = MsToSamples(200, fs);
    int init_len = std::min(N, MsToSamples(2000, fs));

    std::vector<float> init_vals;
    init_vals.reserve(256);
    for (int idx : cand) {
        if (idx < init_len) init_vals.push_back(resp_sm[idx]);
        else break;
    }
    if (init_vals.empty()) return r_peaks;

    float SPKI = Percentile(init_vals, 95);
    float NPKI = Percentile(init_vals, 60);
    float THR1 = NPKI + 0.25f * (SPKI - NPKI);

    std::vector<int> qrs_idx;
    qrs_idx.reserve(cand.size() / 2);
    int last = -1e9;

    for (int idx : cand) {
        if (idx - last < refractory) continue;
        float peak = resp_sm[idx];

        if (peak > THR1) {
            qrs_idx.push_back(idx);
            last = idx;
            SPKI = 0.125f * peak + 0.875f * SPKI;
        } else {
            NPKI = 0.125f * peak + 0.875f * NPKI;
        }
        THR1 = NPKI + 0.25f * (SPKI - NPKI);
    }

    // 6) doprecyzowanie R w raw
    r_peaks.reserve(qrs_idx.size());
    for (int q : qrs_idx) {
        r_peaks.push_back(RefineRPeak(q, raw, x, fs));
    }

    std::sort(r_peaks.begin(), r_peaks.end());
    r_peaks.erase(std::unique(r_peaks.begin(), r_peaks.end()), r_peaks.end());
    return r_peaks;
}


// ==================== RAPORT / METRYKI ====================
struct DetectionMetrics {
    int total_peaks;
    double mean_amplitude;
    double std_amplitude;
    std::string method_name;
};

DetectionMetrics ComputeMetrics(const std::vector<int>& peak_indices,
                                const std::vector<float>& signal,
                                const std::string& method_name) {
    DetectionMetrics metrics;
    metrics.method_name = method_name;
    metrics.total_peaks = static_cast<int>(peak_indices.size());

    if (peak_indices.empty()) {
        metrics.mean_amplitude = 0.0;
        metrics.std_amplitude = 0.0;
        return metrics;
    }

    double sum = 0.0;
    int cnt = 0;
    for (int idx : peak_indices) {
        if (idx >= 0 && idx < static_cast<int>(signal.size())) {
            sum += signal[idx];
            cnt++;
        }
    }
    metrics.mean_amplitude = (cnt > 0) ? (sum / cnt) : 0.0;

    double sum_sq = 0.0;
    for (int idx : peak_indices) {
        if (idx >= 0 && idx < static_cast<int>(signal.size())) {
            double diff = signal[idx] - metrics.mean_amplitude;
            sum_sq += diff * diff;
        }
    }
    double variance = (cnt > 0) ? (sum_sq / cnt) : 0.0;
    metrics.std_amplitude = std::sqrt(std::max(0.0, variance));
    return metrics;
}

void PrintComparisonReport(const std::vector<DetectionMetrics>& all_metrics, int frequency) {
    std::cout << "\n========== RAPORT PORÓWNANIA DETEKCJI PIKÓW R ==========\n";
    std::cout << "Częstotliwość próbkowania: " << frequency << " Hz\n";
    std::cout << "=======================================================\n";

    for (const auto& metrics : all_metrics) {
        std::cout << "\nMetoda: " << metrics.method_name << "\n";
        std::cout << "  Wykryte piki R: " << metrics.total_peaks << "\n";
        std::cout << "  Średnia amplituda: " << metrics.mean_amplitude << "\n";
        std::cout << "  Odchylenie standardowe: " << metrics.std_amplitude << "\n";
    }

    if (all_metrics.size() > 1) {
        int max_peaks = -1;
        std::string max_method;
        for (const auto& m : all_metrics) {
            if (m.total_peaks > max_peaks) {
                max_peaks = m.total_peaks;
                max_method = m.method_name;
            }
        }
        std::cout << "\nMetoda z największą liczbą detekcji: "
                  << max_method << " (" << max_peaks << ")\n";
    }
    std::cout << "=======================================================\n";
}

} // namespace


std::vector<RPeaksAnnotatedSignalDatapoint> RPeaksDetectionService::Detect(
    const std::vector<SignalDatapoint> &datapoints, int frequency) {

    if (datapoints.empty() || frequency <= 0) {
        return {};
    }
    for (const auto& dp : datapoints) {
        if (dp.channelValues.empty()) return {};
    }

    // kanał 0
    std::vector<float> signal(datapoints.size());
    for (size_t i = 0; i < datapoints.size(); ++i) {
        signal[i] = datapoints[i].channelValues[0];
    }

    int fs = frequency;

    // 3 metody
    std::vector<int> peaks_pan_tompkins = DetectPeaksPanTompkins(signal, fs);
    std::vector<int> peaks_hilbert      = DetectPeaksHilbert(signal, fs);
    std::vector<int> peaks_wavelet      = DetectPeaksWavelet(signal, fs);

    // raport
    std::vector<DetectionMetrics> metrics_list;
    metrics_list.push_back(ComputeMetrics(peaks_pan_tompkins, signal, "Pan-Tompkins"));
    metrics_list.push_back(ComputeMetrics(peaks_hilbert, signal, "Transformata Hilberta"));
    metrics_list.push_back(ComputeMetrics(peaks_wavelet, signal, "Falkowa (Wavelet-like)"));
    PrintComparisonReport(metrics_list, fs);

    // wynik główny: Pan-Tompkins
    std::vector<RPeaksAnnotatedSignalDatapoint> result;
    result.reserve(datapoints.size());

    // szybkie oznaczanie pików (O(N) zamiast O(N*M))
    std::vector<char> is_peak(datapoints.size(), 0);
    for (int idx : peaks_pan_tompkins) {
        if (idx >= 0 && idx < static_cast<int>(is_peak.size())) {
            is_peak[idx] = 1;
        }
    }

    for (size_t i = 0; i < datapoints.size(); ++i) {
        RPeaksAnnotatedSignalDatapoint annotated_point;
        annotated_point.channelValues = datapoints[i].channelValues;
        annotated_point.peak = (is_peak[i] != 0);
        result.push_back(annotated_point);
    }

    std::cout << "\n========== PODSUMOWANIE DETEKCJI ==========\n";
    std::cout << "Pan-Tompkins: " << peaks_pan_tompkins.size() << " pików\n";
    std::cout << "Transformata Hilberta: " << peaks_hilbert.size() << " pików\n";
    std::cout << "Falkowa: " << peaks_wavelet.size() << " pików\n";
    std::cout << "Wybrana metoda główna: Pan-Tompkins\n";
    std::cout << "==========================================\n";

    return result;
}
