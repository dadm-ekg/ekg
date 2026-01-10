#include "../../include/service/heart_class_detection_service.h"

#include <QDebug>
#include <QString>
#include <random>

HeartClassResult HeartClassDetectionService::Detect(
    const std::vector<SignalDatapoint> &datapoints,
    const std::vector<RPeaksAnnotatedSignalDatapoint> &r_peaks,
    int frequency
) {
    HeartClassResult result;

    qDebug() << "HeartClassDetectionService called with"
             << datapoints.size() << "datapoints at" << frequency << "Hz";

    if (datapoints.empty()) {
        qDebug() << "[HeartClassDetectionService] No datapoints, returning empty result.";
        return result;
    }

    // const std::string labels[] = {"N", "V", "A"};
    // const int numLabels = 3;

    // int detectionIndex = 0;
    // for (int i = 0; i < static_cast<int>(datapoints.size()); i += 300, ++detectionIndex) {
    //     const std::string &label = labels[detectionIndex % numLabels];
    //     result.annotations.emplace(i, label);
    // }
    std::vector<int> rPeakIndices;
    rPeakIndices.reserve(256);
    for (int i = 0; i < static_cast<int>(datapoints.size()); ++i) {
        if (!r_peaks[i].peaks.empty() && r_peaks[i].peaks[0]) {
            rPeakIndices.push_back(i);
        }
    }

    if (rPeakIndices.empty()) {
        qDebug() << "[HeartClassDetectionService] No R-peaks found, returning empty result.";
        return result;
    }

    // 2) Mock classification: random label per R-peak
    const std::string labels[] = {"N", "V", "A"};
    constexpr int numLabels = 3;

    std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, numLabels - 1);

    for (int peakSampleIndex : rPeakIndices) {
        const std::string &label = labels[dist(rng)];
        result.annotations.emplace(peakSampleIndex, label);
    }


    qDebug() << "[HeartClassDetectionService] Created"
             << result.annotations.size()
             << "detections:";

    for (const auto &entry : result.annotations) {
        const int timestamp = entry.first;               // sample index as timestamp
        const QString label = QString::fromStdString(entry.second);
        qDebug() << "    timestamp/sample" << timestamp << "->" << label;
    }

    return result;
}