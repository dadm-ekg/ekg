#include "../../include/service/r_peaks_detection_service.h"
#include <vector>
#include <iostream>
#include <cmath>

namespace {
    // ==================== METODA PAN-TOMPKINS ====================
    // Własna implementacja algorytmu Pan-Tompkins do detekcji pików R
    std::vector<int> DetectPeaksPanTompkins(const std::vector<float>& signal, int frequency) {
        std::vector<int> peaks;
        
        if (signal.size() < 5) return peaks;
        
        // Krok 1: Różniczkowanie (pochodna pierwszego rzędu)
        std::vector<float> differentiated(signal.size(), 0.0f);
        for (size_t i = 1; i < signal.size(); ++i) {
            differentiated[i] = signal[i] - signal[i - 1];
        }
        
        // Krok 2: Podniesienie do kwadratu
        std::vector<float> squared(signal.size(), 0.0f);
        for (size_t i = 0; i < signal.size(); ++i) {
            squared[i] = differentiated[i] * differentiated[i];
        }
        
        // Krok 3: Wygładzanie oknem ruchomym (Moving Average Filter)
        int window_size = frequency / 10; // ~100 ms okno
        if (window_size < 2) window_size = 2;
        
        std::vector<float> smoothed(signal.size(), 0.0f);
        for (size_t i = 0; i < signal.size(); ++i) {
            float sum = 0.0f;
            int count = 0;
            
            // Obliczanie średniej w oknie
            for (int j = -window_size; j <= window_size; ++j) {
                int idx = static_cast<int>(i) + j;
                if (idx >= 0 && idx < static_cast<int>(signal.size())) {
                    sum += squared[idx];
                    count++;
                }
            }
            smoothed[i] = sum / count;
        }
        
        // Krok 4: Ustalenie progu adaptacyjnego
        float max_value = 0.0f;
        for (size_t i = 0; i < smoothed.size(); ++i) {
            if (smoothed[i] > max_value) {
                max_value = smoothed[i];
            }
        }
        float threshold = 0.5f * max_value;
        
        // Krok 5: Detekcja pików - szukamy maksimów lokalnych
        int min_distance = frequency / 3; // Minimalna odległość między pikami (~200ms dla HR 60-100)
        int last_peak = -min_distance;
        
        for (size_t i = 1; i < smoothed.size() - 1; ++i) {
            // Sprawdzenie czy punkt jest lokalnym maksimum i przekracza próg
            if (smoothed[i] > threshold &&
                smoothed[i] > smoothed[i - 1] &&
                smoothed[i] > smoothed[i + 1] &&
                static_cast<int>(i) - last_peak >= min_distance) {
                peaks.push_back(static_cast<int>(i));
                last_peak = static_cast<int>(i);
            }
        }
        
        return peaks;
    }
    
    // ==================== METODA TRANSFORMATY HILBERTA ====================
    // Własna implementacja transformaty Hilberta dla detekcji obwiedni
    std::vector<int> DetectPeaksHilbert(const std::vector<float>& signal, int frequency) {
        std::vector<int> peaks;
        
        if (signal.size() < 10) return peaks;
        
        // Krok 1: Obliczenie transformaty Hilberta (przybliżenie FIR)
        // Tworzymy sygnał analityczny przez zaburzenie fazy o 90 stopni
        std::vector<float> hilbert_signal(signal.size(), 0.0f);
        
        int hilbert_order = frequency / 5; // Rząd filtru
        if (hilbert_order < 5) hilbert_order = 5;
        if (hilbert_order > static_cast<int>(signal.size() / 2)) {
            hilbert_order = signal.size() / 2;
        }
        
        // Implementacja filtru FIR dla transformaty Hilberta
        for (size_t n = 0; n < signal.size(); ++n) {
            float sum = 0.0f;
            
            for (int k = -hilbert_order; k <= hilbert_order; ++k) {
                int idx = static_cast<int>(n) + k;
                if (idx >= 0 && idx < static_cast<int>(signal.size())) {
                    // Jądro filtru Hilberta
                    if (k % 2 == 1) {
                        float kernel = 2.0f / (3.14159265359f * k);
                        sum += signal[idx] * kernel;
                    }
                }
            }
            hilbert_signal[n] = sum;
        }
        
        // Krok 2: Obliczenie obwiedni (magnitude sygnału analitycznego)
        std::vector<float> envelope(signal.size(), 0.0f);
        for (size_t i = 0; i < signal.size(); ++i) {
            // Sygnał analityczny: x(n) + j*H{x(n)}
            float real_part = signal[i];
            float imag_part = hilbert_signal[i];
            // Magnitude
            envelope[i] = real_part * real_part + imag_part * imag_part;
            envelope[i] = envelope[i] > 0 ? envelope[i] : 0.0f; // Bezpieczny pierwiastek
        }
        
        // Krok 3: Wygładzanie obwiedni
        int smooth_window = frequency / 20;
        if (smooth_window < 2) smooth_window = 2;
        
        std::vector<float> smoothed_envelope(signal.size(), 0.0f);
        for (size_t i = 0; i < signal.size(); ++i) {
            float sum = 0.0f;
            int count = 0;
            
            for (int j = -smooth_window; j <= smooth_window; ++j) {
                int idx = static_cast<int>(i) + j;
                if (idx >= 0 && idx < static_cast<int>(signal.size())) {
                    sum += envelope[idx];
                    count++;
                }
            }
            smoothed_envelope[i] = sum / count;
        }
        
        // Krok 4: Ustalenie progu dla obwiedni
        float max_envelope = 0.0f;
        for (size_t i = 0; i < smoothed_envelope.size(); ++i) {
            if (smoothed_envelope[i] > max_envelope) {
                max_envelope = smoothed_envelope[i];
            }
        }
        float threshold = 0.6f * max_envelope;
        
        // Krok 5: Detekcja pików w obwiedni
        int min_distance = frequency / 3;
        int last_peak = -min_distance;
        
        for (size_t i = 1; i < smoothed_envelope.size() - 1; ++i) {
            if (smoothed_envelope[i] > threshold &&
                smoothed_envelope[i] > smoothed_envelope[i - 1] &&
                smoothed_envelope[i] > smoothed_envelope[i + 1] &&
                static_cast<int>(i) - last_peak >= min_distance) {
                peaks.push_back(static_cast<int>(i));
                last_peak = static_cast<int>(i);
            }
        }
        
        return peaks;
    }
    
    // ==================== METODA FALKOWA (WAVELET) ====================
    // Własna implementacja falkowej detekcji pików R
    std::vector<int> DetectPeaksWavelet(const std::vector<float>& signal, int frequency) {
        std::vector<int> peaks;
        
        if (signal.size() < 15) return peaks;
        
        // Krok 1: Wybór skal falkowych
        int scale_min = frequency / 50;  // ~20 ms
        int scale_max = frequency / 10;  // ~100 ms
        
        if (scale_min < 2) scale_min = 2;
        if (scale_max < scale_min + 1) scale_max = scale_min + 1;
        
        // Krok 2: Transformata falkowa wieloskalowa
        // Używamy falki "Mexican Hat" (LoG - Laplacian of Gaussian)
        std::vector<float> wavelet_response(signal.size(), 0.0f);
        
        for (int scale = scale_min; scale <= scale_max; ++scale) {
            for (size_t i = scale; i < signal.size() - scale; ++i) {
                // Kernel falki Mexican Hat (przybliżenie)
                // d²G/dx² gdzie G to gaussjan
                float second_derivative = 
                    signal[i + scale] - 2.0f * signal[i] + signal[i - scale];
                
                // Ważenie przez skalę
                float wavelet_coeff = second_derivative / (scale * scale);
                
                // Akumulacja odpowiedzi na wszystkich skalach
                wavelet_response[i] += wavelet_coeff * wavelet_coeff;
            }
        }
        
        // Krok 3: Normalizacja odpowiedzi falkowej
        float max_response = 0.0f;
        for (size_t i = 0; i < wavelet_response.size(); ++i) {
            if (wavelet_response[i] > max_response) {
                max_response = wavelet_response[i];
            }
        }
        
        if (max_response > 0.0f) {
            for (size_t i = 0; i < wavelet_response.size(); ++i) {
                wavelet_response[i] /= max_response;
            }
        }
        
        // Krok 4: Wygładzanie odpowiedzi falkowej
        int smooth_window = frequency / 20;
        if (smooth_window < 2) smooth_window = 2;
        
        std::vector<float> smoothed_response(signal.size(), 0.0f);
        for (size_t i = 0; i < signal.size(); ++i) {
            float sum = 0.0f;
            int count = 0;
            
            for (int j = -smooth_window; j <= smooth_window; ++j) {
                int idx = static_cast<int>(i) + j;
                if (idx >= 0 && idx < static_cast<int>(signal.size())) {
                    sum += wavelet_response[idx];
                    count++;
                }
            }
            smoothed_response[i] = sum / count;
        }
        
        // Krok 5: Ustalenie progu adaptacyjnego dla falkowej odpowiedzi
        float threshold = 0.5f;
        
        // Krok 6: Detekcja pików w odpowiedzi falkowej
        int min_distance = frequency / 3;
        int last_peak = -min_distance;
        
        for (size_t i = 1; i < smoothed_response.size() - 1; ++i) {
            if (smoothed_response[i] > threshold &&
                smoothed_response[i] > smoothed_response[i - 1] &&
                smoothed_response[i] > smoothed_response[i + 1] &&
                static_cast<int>(i) - last_peak >= min_distance) {
                peaks.push_back(static_cast<int>(i));
                last_peak = static_cast<int>(i);
            }
        }
        
        return peaks;
    }
    
    // ==================== PORÓWNANIE METOD ====================
    struct DetectionMetrics {
        int total_peaks;
        double mean_amplitude;
        double std_amplitude;
        std::string method_name;
    };
    
    // Obliczenie miary dla każdej metody
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
        
        // Obliczenie średniej amplitudy
        double sum = 0.0;
        for (int idx : peak_indices) {
            if (idx >= 0 && idx < static_cast<int>(signal.size())) {
                sum += signal[idx];
            }
        }
        metrics.mean_amplitude = sum / peak_indices.size();
        
        // Obliczenie odchylenia standardowego
        double sum_sq = 0.0;
        for (int idx : peak_indices) {
            if (idx >= 0 && idx < static_cast<int>(signal.size())) {
                double diff = signal[idx] - metrics.mean_amplitude;
                sum_sq += diff * diff;
            }
        }
        
        double variance = sum_sq / peak_indices.size();
        metrics.std_amplitude = variance > 0.0 ? variance : 0.0;
    }
    
    // Drukowanie raportu porównawczego
    void PrintComparisonReport(const std::vector<DetectionMetrics>& all_metrics,
                              int frequency) {
        std::cout << "\n========== RAPORT PORÓWNANIA DETEKCJI PIKÓW R ==========" << std::endl;
        std::cout << "Częstotliwość próbkowania: " << frequency << " Hz" << std::endl;
        std::cout << "=======================================================" << std::endl;
        
        for (const auto& metrics : all_metrics) {
            std::cout << "\nMetoda: " << metrics.method_name << std::endl;
            std::cout << "  Wykryte piki R: " << metrics.total_peaks << std::endl;
            std::cout << "  Średnia amplituda: " << metrics.mean_amplitude << std::endl;
            std::cout << "  Odchylenie standardowe: " << metrics.std_amplitude << std::endl;
        }
        
        // Statystyki porównawcze
        if (all_metrics.size() > 1) {
            std::cout << "\n--- Statystyki Porównawcze ---" << std::endl;
            
            int max_peaks = 0;
            std::string max_method;
            for (const auto& m : all_metrics) {
                if (m.total_peaks > max_peaks) {
                    max_peaks = m.total_peaks;
                    max_method = m.method_name;
                }
            }
            std::cout << "Metoda z największą liczbą detekcji: " << max_method
                     << " (" << max_peaks << " pików)" << std::endl;
            
            // Obliczenie zgodności między metodami
            if (all_metrics.size() == 3) {
                int total_detected = all_metrics[0].total_peaks +
                                    all_metrics[1].total_peaks +
                                    all_metrics[2].total_peaks;
                double avg_peaks = total_detected / 3.0;
                std::cout << "Średnia liczba pików między metodami: " << avg_peaks << std::endl;
                
                // Wariancja liczby pików
                double variance = 0.0;
                for (const auto& m : all_metrics) {
                    double diff = m.total_peaks - avg_peaks;
                    variance += diff * diff;
                }
                variance /= 3.0;
                std::cout << "Wariancja liczby wykrytych pików: " << variance << std::endl;
            }
        }
        
        std::cout << "=======================================================" << std::endl;
    }
}

std::vector<RPeaksAnnotatedSignalDatapoint> RPeaksDetectionService::Detect(
    const std::vector<SignalDatapoint> &datapoints, int frequency) {
    
    // Walidacja wejścia
    if (datapoints.empty() || frequency <= 0) {
        return std::vector<RPeaksAnnotatedSignalDatapoint>{};
    }
    
    // Sprawdzenie czy wszystkie punkty mają wartości kanałów
    for (const auto& dp : datapoints) {
        if (dp.channelValues.empty()) {
            return std::vector<RPeaksAnnotatedSignalDatapoint>{};
        }
    }
    
    // Krok 1: Ekstrakcja sygnału z pierwszego kanału
    std::vector<float> signal(datapoints.size());
    for (size_t i = 0; i < datapoints.size(); ++i) {
        signal[i] = datapoints[i].channelValues[0];
    }
    
    // Krok 2: Normalizacja częstotliwości
    int normalized_frequency = frequency;
    
    // Krok 3: Detekcja pików trzema różnymi metodami
    std::vector<int> peaks_pan_tompkins = DetectPeaksPanTompkins(signal, normalized_frequency);
    std::vector<int> peaks_hilbert = DetectPeaksHilbert(signal, normalized_frequency);
    std::vector<int> peaks_wavelet = DetectPeaksWavelet(signal, normalized_frequency);
    
    // Krok 4: Obliczenie miar dla każdej metody
    std::vector<DetectionMetrics> metrics_list;
    metrics_list.push_back(ComputeMetrics(peaks_pan_tompkins, signal, "Pan-Tompkins"));
    metrics_list.push_back(ComputeMetrics(peaks_hilbert, signal, "Transformata Hilberta"));
    metrics_list.push_back(ComputeMetrics(peaks_wavelet, signal, "Falkowa (Wavelet)"));
    
    // Krok 5: Wydruk raportu porównawczego
    PrintComparisonReport(metrics_list, normalized_frequency);
    
    // Krok 6: Tworzenie wektora wyjściowego z zaznaczonymi pikami
    // Używamy metody Pan-Tompkins jako głównej
    std::vector<RPeaksAnnotatedSignalDatapoint> result;
    result.reserve(datapoints.size());
    
    for (size_t i = 0; i < datapoints.size(); ++i) {
        RPeaksAnnotatedSignalDatapoint annotated_point;
        annotated_point.channelValues = datapoints[i].channelValues;
        
        // Zaznaczenie piku jeśli obecny indeks znajduje się w wektorze pików
        annotated_point.peak = false;
        for (int peak_idx : peaks_pan_tompkins) {
            if (peak_idx == static_cast<int>(i)) {
                annotated_point.peak = true;
                break;
            }
        }
        
        result.push_back(annotated_point);
    }
    
    // Krok 7: Podsumowanie wyników
    std::cout << "\n========== PODSUMOWANIE DETEKCJI ==========" << std::endl;
    std::cout << "Pan-Tompkins: " << peaks_pan_tompkins.size() << " pików" << std::endl;
    std::cout << "Transformata Hilberta: " << peaks_hilbert.size() << " pików" << std::endl;
    std::cout << "Falkowa: " << peaks_wavelet.size() << " pików" << std::endl;
    std::cout << "Wybrana metoda główna: Pan-Tompkins" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    return result;
}
