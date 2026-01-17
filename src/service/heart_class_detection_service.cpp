// #include "../../include/service/heart_class_detection_service.h"

// #include <QDebug>
// #include <QFile>

// #include <algorithm>
// #include <cmath>
// #include <mutex>
// #include <random>
// #include <string>
// #include <vector>

// namespace {

// // ===== Model config (from notebooks) =====
// constexpr int HALF_WINDOW = 30;
// constexpr int WINDOW_LEN  = 2 * HALF_WINDOW + 1; // 61

// constexpr int CONV1_CIN=1,  CONV1_COUT=8,  CONV1_K=7, CONV1_PAD=3;
// constexpr int CONV2_CIN=8,  CONV2_COUT=16, CONV2_K=5, CONV2_PAD=2;
// constexpr int CONV3_CIN=16, CONV3_COUT=32, CONV3_K=3, CONV3_PAD=1;
// constexpr int CONV4_CIN=32, CONV4_COUT=32, CONV4_K=3, CONV4_PAD=1;
// constexpr int CONV5_CIN=32, CONV5_COUT=32, CONV5_K=3, CONV5_PAD=1;

// constexpr int FC_IN = 32;
// constexpr int FC_OUT = 4;

// // Training notebook uses these class names/order:
// static const char* CLASS_NAMES[FC_OUT] = {"N", "S", "V", "other"};

// // ===== CSV loading =====
// // Training notebook exports CSV as:
// // np.savetxt(f"{k}.csv", arr.reshape(arr.shape[0], -1), delimiter=",")
// // so conv_w is (Cout, Cin*K), conv_b is (Cout, 1), fc_w is (4, 32), fc_b is (4, 1).

// bool LoadCsvFlat(const QString& path, std::vector<float>& out) {
//     QFile file(path);
//     if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
//         qWarning() << "[HeartClassDetectionService] Failed to open weights file:" << path;
//         return false;
//     }

//     const QByteArray bytes = file.readAll();
//     const char* p = bytes.constData();
//     const char* end = p + bytes.size();

//     out.clear();
//     out.reserve(bytes.size() / 6); // rough

//     while (p < end) {
//         while (p < end && (*p == ',' || *p == '\n' || *p == '\r' || *p == ' ' || *p == '\t')) ++p;
//         if (p >= end) break;

//         char* next = nullptr;
//         float v = std::strtof(p, &next);
//         if (next == p) { ++p; continue; }
//         out.push_back(v);
//         p = next;
//     }

//     return !out.empty();
// }

// struct Weights {
//     std::vector<float> conv1_w, conv1_b;
//     std::vector<float> conv2_w, conv2_b;
//     std::vector<float> conv3_w, conv3_b;
//     std::vector<float> conv4_w, conv4_b;
//     std::vector<float> conv5_w, conv5_b;
//     std::vector<float> fc_w, fc_b;
//     bool ok = false;
// };

// inline int CountConv(int cout, int cin, int k) { return cout * cin * k; }
// inline int CountBias(int cout) { return cout; }
// inline int CountFcW(int out, int in) { return out * in; }

// bool LoadAllWeights(Weights& W) {
//     // Put the CSVs into Qt resources under this prefix (instructions below).
//     const QString base = ":/models/beatcnn5/csv/";

//     struct Spec { const char* name; std::vector<float>* dst; int expected; };
//     const Spec specs[] = {
//         {"conv1_w.csv", &W.conv1_w, CountConv(CONV1_COUT, CONV1_CIN, CONV1_K)},
//         {"conv1_b.csv", &W.conv1_b, CountBias(CONV1_COUT)},
//         {"conv2_w.csv", &W.conv2_w, CountConv(CONV2_COUT, CONV2_CIN, CONV2_K)},
//         {"conv2_b.csv", &W.conv2_b, CountBias(CONV2_COUT)},
//         {"conv3_w.csv", &W.conv3_w, CountConv(CONV3_COUT, CONV3_CIN, CONV3_K)},
//         {"conv3_b.csv", &W.conv3_b, CountBias(CONV3_COUT)},
//         {"conv4_w.csv", &W.conv4_w, CountConv(CONV4_COUT, CONV4_CIN, CONV4_K)},
//         {"conv4_b.csv", &W.conv4_b, CountBias(CONV4_COUT)},
//         {"conv5_w.csv", &W.conv5_w, CountConv(CONV5_COUT, CONV5_CIN, CONV5_K)},
//         {"conv5_b.csv", &W.conv5_b, CountBias(CONV5_COUT)},
//         {"fc_w.csv",    &W.fc_w,    CountFcW(FC_OUT, FC_IN)},
//         {"fc_b.csv",    &W.fc_b,    CountBias(FC_OUT)},
//     };

//     for (const auto& s : specs) {
//         std::vector<float> tmp;
//         const QString path = base + s.name;

//         if (!LoadCsvFlat(path, tmp)) {
//             qWarning() << "[HeartClassDetectionService] Missing/empty weights:" << path;
//             return false;
//         }

//         if (static_cast<int>(tmp.size()) != s.expected) {
//             qWarning() << "[HeartClassDetectionService] Wrong weight size for" << path
//                        << "got" << tmp.size() << "expected" << s.expected;
//             return false;
//         }

//         *s.dst = std::move(tmp);
//     }
//     return true;
// }

// Weights gW;
// std::once_flag gWOnce;

// // ===== Inference ops =====

// inline float Relu(float x) { return x > 0.f ? x : 0.f; }

// // Layout: [C][L] contiguous, idx = c*L + i
// std::vector<float> Conv1dSame(
//     const std::vector<float>& x, int Cin, int L,
//     const std::vector<float>& w, const std::vector<float>& b,
//     int Cout, int K, int pad
// ) {
//     std::vector<float> out(static_cast<size_t>(Cout) * L, 0.f);

//     for (int oc = 0; oc < Cout; ++oc) {
//         for (int i = 0; i < L; ++i) {
//             float acc = b[oc];
//             for (int ic = 0; ic < Cin; ++ic) {
//                 const float* xRow = &x[static_cast<size_t>(ic) * L];
//                 // weights are stored per oc as flatten(Cin*K), ic-major then k
//                 const float* wRow = &w[(static_cast<size_t>(oc) * Cin + ic) * K];
//                 for (int k = 0; k < K; ++k) {
//                     const int xi = i + k - pad;
//                     if (xi >= 0 && xi < L) {
//                         acc += xRow[xi] * wRow[k];
//                     }
//                 }
//             }
//             out[static_cast<size_t>(oc) * L + i] = acc;
//         }
//     }
//     return out;
// }

// std::vector<float> ReluInPlace(std::vector<float> x) {
//     for (float& v : x) v = Relu(v);
//     return x;
// }

// std::vector<float> MaxPool1d(const std::vector<float>& x, int C, int L, int k=2, int s=2) {
//     const int Lout = (L - k) / s + 1;
//     std::vector<float> out(static_cast<size_t>(C) * Lout, 0.f);

//     for (int c = 0; c < C; ++c) {
//         const float* inRow = &x[static_cast<size_t>(c) * L];
//         float* outRow = &out[static_cast<size_t>(c) * Lout];
//         for (int i = 0; i < Lout; ++i) {
//             const int start = i * s;
//             float m = inRow[start];
//             for (int j = 1; j < k; ++j) m = std::max(m, inRow[start + j]);
//             outRow[i] = m;
//         }
//     }
//     return out;
// }

// std::vector<float> GlobalAvgPool(const std::vector<float>& x, int C, int L) {
//     std::vector<float> out(C, 0.f);
//     for (int c = 0; c < C; ++c) {
//         const float* row = &x[static_cast<size_t>(c) * L];
//         float sum = 0.f;
//         for (int i = 0; i < L; ++i) sum += row[i];
//         out[c] = sum / static_cast<float>(L);
//     }
//     return out;
// }

// std::vector<float> Softmax(const std::vector<float>& logits) {
//     float mx = *std::max_element(logits.begin(), logits.end());
//     std::vector<float> exps(logits.size(), 0.f);
//     float sum = 0.f;
//     for (size_t i = 0; i < logits.size(); ++i) {
//         exps[i] = std::exp(logits[i] - mx);
//         sum += exps[i];
//     }
//     if (sum <= 0.f) sum = 1.f;
//     for (float& v : exps) v /= sum;
//     return exps;
// }

// int ArgMax(const std::vector<float>& v) {
//     return static_cast<int>(std::distance(v.begin(), std::max_element(v.begin(), v.end())));
// }

// // Forward one 61-sample window (single channel)
// std::vector<float> ForwardOne(const std::vector<float>& window61, const Weights& W) {
//     // x: (1, 61)
//     std::vector<float> x = window61;

//     x = ReluInPlace(Conv1dSame(x, CONV1_CIN, WINDOW_LEN, W.conv1_w, W.conv1_b, CONV1_COUT, CONV1_K, CONV1_PAD));
//     x = MaxPool1d(x, CONV1_COUT, WINDOW_LEN);
//     int L = (WINDOW_LEN - 2) / 2 + 1; // 30

//     x = ReluInPlace(Conv1dSame(x, CONV2_CIN, L, W.conv2_w, W.conv2_b, CONV2_COUT, CONV2_K, CONV2_PAD));
//     x = MaxPool1d(x, CONV2_COUT, L);
//     L = (L - 2) / 2 + 1; // 15

//     x = ReluInPlace(Conv1dSame(x, CONV3_CIN, L, W.conv3_w, W.conv3_b, CONV3_COUT, CONV3_K, CONV3_PAD));
//     x = ReluInPlace(Conv1dSame(x, CONV4_CIN, L, W.conv4_w, W.conv4_b, CONV4_COUT, CONV4_K, CONV4_PAD));
//     x = ReluInPlace(Conv1dSame(x, CONV5_CIN, L, W.conv5_w, W.conv5_b, CONV5_COUT, CONV5_K, CONV5_PAD));

//     const std::vector<float> feat = GlobalAvgPool(x, CONV5_COUT, L);

//     std::vector<float> logits(FC_OUT, 0.f);
//     for (int o = 0; o < FC_OUT; ++o) {
//         float acc = W.fc_b[o];
//         const float* wrow = &W.fc_w[static_cast<size_t>(o) * FC_IN];
//         for (int i = 0; i < FC_IN; ++i) acc += wrow[i] * feat[i];
//         logits[o] = acc;
//     }

//     return Softmax(logits);
// }

// } // namespace

// HeartClassResult HeartClassDetectionService::Detect(
//     const std::vector<SignalDatapoint>& datapoints,
//     const std::vector<RPeaksAnnotatedSignalDatapoint>& r_peaks,
//     int frequency
// ) {
//     HeartClassResult result;

//     qDebug() << "HeartClassDetectionService called with"
//              << datapoints.size() << "datapoints at" << frequency << "Hz";

//     if (datapoints.empty() || r_peaks.empty()) {
//         qDebug() << "[HeartClassDetectionService] Empty input, returning empty result.";
//         return result;
//     }

//     // Load weights once (thread-safe)
//     std::call_once(gWOnce, []() {
//         Weights tmp;
//         tmp.ok = LoadAllWeights(tmp);
//         if (!tmp.ok) {
//             qWarning() << "[HeartClassDetectionService] BeatCNN5 weights not loaded.";
//         }
//         gW = std::move(tmp);
//     });

//     if (!gW.ok) {
//         return result; // no fallback mock
//     }

//     const int ch = 0;
//     const int L = std::min<int>(datapoints.size(), r_peaks.size());
//     if (L <= 0) return result;

//     // Record-level z-score (like notebook preprocess_signal after filtering)
//     double sum = 0.0, sumsq = 0.0;
//     int count = 0;
//     for (int i = 0; i < L; ++i) {
//         if (static_cast<int>(datapoints[i].channelValues.size()) > ch) {
//             const double v = datapoints[i].channelValues[ch];
//             sum += v;
//             sumsq += v * v;
//             ++count;
//         }
//     }
//     const double mean = (count > 0) ? (sum / count) : 0.0;
//     const double var  = (count > 0) ? std::max(0.0, (sumsq / count) - (mean * mean)) : 0.0;
//     const double stdev = std::sqrt(var) + 1e-8;

//     // Collect peaks (your corrected approach)
//     std::vector<int> rPeakIndices;
//     rPeakIndices.reserve(256);
//     for (int i = 0; i < L; ++i) {
//         if (r_peaks[i].HasPeak(ch)) {
//             rPeakIndices.push_back(i);
//         }
//     }
//     if (rPeakIndices.empty()) {
//         qDebug() << "[HeartClassDetectionService] No R-peaks found, returning empty result.";
//         return result;
//     }

//     // Classify each R-peak
//     for (int peakSampleIndex : rPeakIndices) {
//         std::vector<float> window(WINDOW_LEN, 0.f);
//         const int start = peakSampleIndex - HALF_WINDOW;

//         for (int j = 0; j < WINDOW_LEN; ++j) {
//             const int si = start + j;
//             if (si >= 0 && si < L) {
//                 float v = 0.f;
//                 if (static_cast<int>(datapoints[si].channelValues.size()) > ch) {
//                     v = datapoints[si].channelValues[ch];
//                 }
//                 window[j] = static_cast<float>((static_cast<double>(v) - mean) / stdev);
//             }
//         }

//         const std::vector<float> probs = ForwardOne(window, gW);
//         const int pred = ArgMax(probs);
//         result.annotations.emplace(peakSampleIndex, std::string(CLASS_NAMES[pred]));
//     }

//     qDebug() << "[HeartClassDetectionService] Created"
//              << result.annotations.size() << "beat classifications.";

//     return result;
// }

#include "../../include/service/heart_class_detection_service.h"

#include <QDebug>
#include <QFile>

#include <algorithm>
#include <cmath>
#include <cstring>   // std::memcpy
#include <mutex>
#include <string>
#include <vector>

namespace {

// ===== Model config (from notebooks) =====
constexpr int HALF_WINDOW = 30;
constexpr int WINDOW_LEN  = 2 * HALF_WINDOW + 1; // 61

constexpr int CONV1_CIN=1,  CONV1_COUT=8,  CONV1_K=7, CONV1_PAD=3;
constexpr int CONV2_CIN=8,  CONV2_COUT=16, CONV2_K=5, CONV2_PAD=2;
constexpr int CONV3_CIN=16, CONV3_COUT=32, CONV3_K=3, CONV3_PAD=1;
constexpr int CONV4_CIN=32, CONV4_COUT=32, CONV4_K=3, CONV4_PAD=1;
constexpr int CONV5_CIN=32, CONV5_COUT=32, CONV5_K=3, CONV5_PAD=1;

constexpr int FC_IN = 32;
constexpr int FC_OUT = 4;

// Training notebook uses these class names/order:
static const char* CLASS_NAMES[FC_OUT] = {"N", "S", "V", "other"};

// ===== Packed binary layout (hardcoded; no JSON needed) =====
//
// Order in beatcnn5_weights.bin:
// conv1_w, conv1_b, conv2_w, conv2_b, conv3_w, conv3_b, conv4_w, conv4_b, conv5_w, conv5_b, fc_w, fc_b
//
// Counts (float32):
// conv1_w: 8*1*7   = 56
// conv1_b: 8       = 8
// conv2_w: 16*8*5  = 640
// conv2_b: 16      = 16
// conv3_w: 32*16*3 = 1536
// conv3_b: 32      = 32
// conv4_w: 32*32*3 = 3072
// conv4_b: 32      = 32
// conv5_w: 32*32*3 = 3072
// conv5_b: 32      = 32
// fc_w:    4*32    = 128
// fc_b:    4       = 4
//
// Total floats = 8628  => total bytes = 34512

constexpr int CONV1_W_N = CONV1_COUT * CONV1_CIN * CONV1_K; // 56
constexpr int CONV1_B_N = CONV1_COUT;                       // 8
constexpr int CONV2_W_N = CONV2_COUT * CONV2_CIN * CONV2_K; // 640
constexpr int CONV2_B_N = CONV2_COUT;                       // 16
constexpr int CONV3_W_N = CONV3_COUT * CONV3_CIN * CONV3_K; // 1536
constexpr int CONV3_B_N = CONV3_COUT;                       // 32
constexpr int CONV4_W_N = CONV4_COUT * CONV4_CIN * CONV4_K; // 3072
constexpr int CONV4_B_N = CONV4_COUT;                       // 32
constexpr int CONV5_W_N = CONV5_COUT * CONV5_CIN * CONV5_K; // 3072
constexpr int CONV5_B_N = CONV5_COUT;                       // 32
constexpr int FC_W_N    = FC_OUT * FC_IN;                   // 128
constexpr int FC_B_N    = FC_OUT;                           // 4

constexpr int OFF_CONV1_W = 0;
constexpr int OFF_CONV1_B = OFF_CONV1_W + CONV1_W_N;                 // 56
constexpr int OFF_CONV2_W = OFF_CONV1_B + CONV1_B_N;                 // 64
constexpr int OFF_CONV2_B = OFF_CONV2_W + CONV2_W_N;                 // 704
constexpr int OFF_CONV3_W = OFF_CONV2_B + CONV2_B_N;                 // 720
constexpr int OFF_CONV3_B = OFF_CONV3_W + CONV3_W_N;                 // 2256
constexpr int OFF_CONV4_W = OFF_CONV3_B + CONV3_B_N;                 // 2288
constexpr int OFF_CONV4_B = OFF_CONV4_W + CONV4_W_N;                 // 5360
constexpr int OFF_CONV5_W = OFF_CONV4_B + CONV4_B_N;                 // 5392
constexpr int OFF_CONV5_B = OFF_CONV5_W + CONV5_W_N;                 // 8464
constexpr int OFF_FC_W    = OFF_CONV5_B + CONV5_B_N;                 // 8496
constexpr int OFF_FC_B    = OFF_FC_W + FC_W_N;                       // 8624

constexpr int TOTAL_FLOATS = OFF_FC_B + FC_B_N;                      // 8628
constexpr int TOTAL_BYTES  = TOTAL_FLOATS * static_cast<int>(sizeof(float)); // 34512

struct Weights {
    std::vector<float> blob; // owns all weights

    const float* conv1_w = nullptr; const float* conv1_b = nullptr;
    const float* conv2_w = nullptr; const float* conv2_b = nullptr;
    const float* conv3_w = nullptr; const float* conv3_b = nullptr;
    const float* conv4_w = nullptr; const float* conv4_b = nullptr;
    const float* conv5_w = nullptr; const float* conv5_b = nullptr;
    const float* fc_w    = nullptr; const float* fc_b    = nullptr;

    bool ok = false;
};

bool LoadWeightsBinary(Weights& W) {
    // Put the BIN into Qt resources under this path:
    const QString path = ":/models/beatcnn5/beatcnn5_weights.bin";

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[HeartClassDetectionService] Failed to open weights bin:" << path;
        return false;
    }

    const QByteArray bytes = file.readAll();
    if (bytes.size() != TOTAL_BYTES) {
        qWarning() << "[HeartClassDetectionService] Wrong weights size for" << path
                   << "got" << bytes.size() << "expected" << TOTAL_BYTES;
        return false;
    }

    W.blob.resize(TOTAL_FLOATS);
    std::memcpy(W.blob.data(), bytes.constData(), static_cast<size_t>(TOTAL_BYTES));

    // Create views
    const float* base = W.blob.data();
    W.conv1_w = base + OFF_CONV1_W; W.conv1_b = base + OFF_CONV1_B;
    W.conv2_w = base + OFF_CONV2_W; W.conv2_b = base + OFF_CONV2_B;
    W.conv3_w = base + OFF_CONV3_W; W.conv3_b = base + OFF_CONV3_B;
    W.conv4_w = base + OFF_CONV4_W; W.conv4_b = base + OFF_CONV4_B;
    W.conv5_w = base + OFF_CONV5_W; W.conv5_b = base + OFF_CONV5_B;
    W.fc_w    = base + OFF_FC_W;    W.fc_b    = base + OFF_FC_B;

    return true;
}

Weights gW;
std::once_flag gWOnce;

// ===== Inference ops =====

inline float Relu(float x) { return x > 0.f ? x : 0.f; }

// Layout: input x is [Cin][L] contiguous, idx = ic*L + i
// Weights are [Cout][Cin][K] contiguous, idx = (oc*Cin + ic)*K + k
std::vector<float> Conv1dSame(
    const std::vector<float>& x, int Cin, int L,
    const float* w, const float* b,
    int Cout, int K, int pad
) {
    std::vector<float> out(static_cast<size_t>(Cout) * L, 0.f);

    for (int oc = 0; oc < Cout; ++oc) {
        for (int i = 0; i < L; ++i) {
            float acc = b[oc];
            for (int ic = 0; ic < Cin; ++ic) {
                const float* xRow = &x[static_cast<size_t>(ic) * L];
                const float* wRow = &w[(static_cast<size_t>(oc) * Cin + ic) * K];
                for (int k = 0; k < K; ++k) {
                    const int xi = i + k - pad;
                    if (xi >= 0 && xi < L) {
                        acc += xRow[xi] * wRow[k];
                    }
                }
            }
            out[static_cast<size_t>(oc) * L + i] = acc;
        }
    }
    return out;
}

std::vector<float> ReluInPlace(std::vector<float> x) {
    for (float& v : x) v = Relu(v);
    return x;
}

std::vector<float> MaxPool1d(const std::vector<float>& x, int C, int L, int k=2, int s=2) {
    const int Lout = (L - k) / s + 1;
    std::vector<float> out(static_cast<size_t>(C) * Lout, 0.f);

    for (int c = 0; c < C; ++c) {
        const float* inRow = &x[static_cast<size_t>(c) * L];
        float* outRow = &out[static_cast<size_t>(c) * Lout];
        for (int i = 0; i < Lout; ++i) {
            const int start = i * s;
            float m = inRow[start];
            for (int j = 1; j < k; ++j) m = std::max(m, inRow[start + j]);
            outRow[i] = m;
        }
    }
    return out;
}

std::vector<float> GlobalAvgPool(const std::vector<float>& x, int C, int L) {
    std::vector<float> out(C, 0.f);
    for (int c = 0; c < C; ++c) {
        const float* row = &x[static_cast<size_t>(c) * L];
        float sum = 0.f;
        for (int i = 0; i < L; ++i) sum += row[i];
        out[c] = sum / static_cast<float>(L);
    }
    return out;
}

std::vector<float> Softmax(const std::vector<float>& logits) {
    float mx = *std::max_element(logits.begin(), logits.end());
    std::vector<float> exps(logits.size(), 0.f);
    float sum = 0.f;
    for (size_t i = 0; i < logits.size(); ++i) {
        exps[i] = std::exp(logits[i] - mx);
        sum += exps[i];
    }
    if (sum <= 0.f) sum = 1.f;
    for (float& v : exps) v /= sum;
    return exps;
}

int ArgMax(const std::vector<float>& v) {
    return static_cast<int>(std::distance(v.begin(), std::max_element(v.begin(), v.end())));
}

// Forward one 61-sample window (single channel)
std::vector<float> ForwardOne(const std::vector<float>& window61, const Weights& W) {
    std::vector<float> x = window61; // (1, 61)

    x = ReluInPlace(Conv1dSame(x, CONV1_CIN, WINDOW_LEN, W.conv1_w, W.conv1_b, CONV1_COUT, CONV1_K, CONV1_PAD));
    x = MaxPool1d(x, CONV1_COUT, WINDOW_LEN);
    int L = (WINDOW_LEN - 2) / 2 + 1; // 30

    x = ReluInPlace(Conv1dSame(x, CONV2_CIN, L, W.conv2_w, W.conv2_b, CONV2_COUT, CONV2_K, CONV2_PAD));
    x = MaxPool1d(x, CONV2_COUT, L);
    L = (L - 2) / 2 + 1; // 15

    x = ReluInPlace(Conv1dSame(x, CONV3_CIN, L, W.conv3_w, W.conv3_b, CONV3_COUT, CONV3_K, CONV3_PAD));
    x = ReluInPlace(Conv1dSame(x, CONV4_CIN, L, W.conv4_w, W.conv4_b, CONV4_COUT, CONV4_K, CONV4_PAD));
    x = ReluInPlace(Conv1dSame(x, CONV5_CIN, L, W.conv5_w, W.conv5_b, CONV5_COUT, CONV5_K, CONV5_PAD));

    const std::vector<float> feat = GlobalAvgPool(x, CONV5_COUT, L);

    std::vector<float> logits(FC_OUT, 0.f);
    for (int o = 0; o < FC_OUT; ++o) {
        float acc = W.fc_b[o];
        const float* wrow = &W.fc_w[static_cast<size_t>(o) * FC_IN];
        for (int i = 0; i < FC_IN; ++i) acc += wrow[i] * feat[i];
        logits[o] = acc;
    }

    return Softmax(logits);
}

} // namespace

HeartClassResult HeartClassDetectionService::Detect(
    const std::vector<SignalDatapoint>& datapoints,
    const std::vector<RPeaksAnnotatedSignalDatapoint>& r_peaks,
    int frequency
) {
    HeartClassResult result;

    qDebug() << "HeartClassDetectionService called with"
             << datapoints.size() << "datapoints at" << frequency << "Hz";

    if (datapoints.empty() || r_peaks.empty()) {
        qDebug() << "[HeartClassDetectionService] Empty input, returning empty result.";
        return result;
    }

    // Load weights once (thread-safe)
    std::call_once(gWOnce, []() {
        Weights tmp;
        tmp.ok = LoadWeightsBinary(tmp);
        if (!tmp.ok) {
            qWarning() << "[HeartClassDetectionService] BeatCNN5 weights not loaded (bin).";
        }
        gW = std::move(tmp);
    });

    if (!gW.ok) {
        return result;
    }

    const int ch = 0;
    const int L = std::min<int>(datapoints.size(), r_peaks.size());
    if (L <= 0) return result;

    // Record-level z-score (like notebook preprocess_signal after filtering)
    double sum = 0.0, sumsq = 0.0;
    int count = 0;
    for (int i = 0; i < L; ++i) {
        if (static_cast<int>(datapoints[i].channelValues.size()) > ch) {
            const double v = datapoints[i].channelValues[ch];
            sum += v;
            sumsq += v * v;
            ++count;
        }
    }
    const double mean = (count > 0) ? (sum / count) : 0.0;
    const double var  = (count > 0) ? std::max(0.0, (sumsq / count) - (mean * mean)) : 0.0;
    const double stdev = std::sqrt(var) + 1e-8;

    // Collect peaks
    std::vector<int> rPeakIndices;
    rPeakIndices.reserve(256);
    for (int i = 0; i < L; ++i) {
        if (r_peaks[i].HasPeak(ch)) {
            rPeakIndices.push_back(i);
        }
    }
    if (rPeakIndices.empty()) {
        qDebug() << "[HeartClassDetectionService] No R-peaks found, returning empty result.";
        return result;
    }

    // Classify each R-peak
    for (int peakSampleIndex : rPeakIndices) {
        std::vector<float> window(WINDOW_LEN, 0.f);
        const int start = peakSampleIndex - HALF_WINDOW;

        for (int j = 0; j < WINDOW_LEN; ++j) {
            const int si = start + j;
            if (si >= 0 && si < L) {
                float v = 0.f;
                if (static_cast<int>(datapoints[si].channelValues.size()) > ch) {
                    v = datapoints[si].channelValues[ch];
                }
                window[j] = static_cast<float>((static_cast<double>(v) - mean) / stdev);
            }
        }

        const std::vector<float> probs = ForwardOne(window, gW);
        const int pred = ArgMax(probs);
        result.annotations.emplace(peakSampleIndex, std::string(CLASS_NAMES[pred]));
    }

    qDebug() << "[HeartClassDetectionService] Created"
             << result.annotations.size() << "beat classifications.";

    return result;
}
