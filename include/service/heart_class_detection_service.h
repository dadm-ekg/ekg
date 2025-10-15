#ifndef EKG_HEART_CLASS_DETECTION_SERVICE_H
#define EKG_HEART_CLASS_DETECTION_SERVICE_H
#include <vector>

#include "../dto/heart_class_result.h"
#include "../model/signal_datapoint.h"

class IHeartClassDetectionService {
public:
    virtual ~IHeartClassDetectionService() = default;

    virtual HeartClassResult Detect(std::vector<SignalDatapoint> datapoints, int frequency) = 0;
};

#endif //EKG_HEART_CLASS_DETECTION_SERVICE_H
