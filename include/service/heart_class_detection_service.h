#ifndef EKG_HEART_CLASS_DETECTION_SERVICE_IMPL_H
#define EKG_HEART_CLASS_DETECTION_SERVICE_IMPL_H

#include "abstract/heart_class_detection_service.h"

class HeartClassDetectionService : public IHeartClassDetectionService {
public:
    HeartClassResult Detect(const std::vector<SignalDatapoint>& datapoints, int frequency) override;
};

#endif //EKG_HEART_CLASS_DETECTION_SERVICE_IMPL_H
