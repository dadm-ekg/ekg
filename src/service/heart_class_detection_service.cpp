#include "../../include/service/heart_class_detection_service.h"

#include <QDebug>
#include <QString>

HeartClassResult HeartClassDetectionService::Detect(
    const std::vector<SignalDatapoint> &datapoints,
    int frequency
) {
    HeartClassResult result;

    qDebug() << "HeartClassDetectionService called with"
             << datapoints.size() << "datapoints at" << frequency << "Hz";

    if (datapoints.empty()) {
        qDebug() << "[HeartClassDetectionService] No datapoints, returning empty result.";
        return result;
    }

    const std::string labels[] = {"N", "V", "A"};
    const int numLabels = 3;

    int detectionIndex = 0;
    for (int i = 0; i < static_cast<int>(datapoints.size()); i += 300, ++detectionIndex) {
        const std::string &label = labels[detectionIndex % numLabels];
        result.annotations.emplace(i, label);
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