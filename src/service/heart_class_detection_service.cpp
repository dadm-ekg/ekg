#include "../../include/service/heart_class_detection_service.h"

class HeartClassDetectionService : public IHeartClassDetectionService {
public:
    HeartClassResult Detect(std::vector<SignalDatapoint> datapoints, int frequency) override {
        // TODO(Jeremiasz): trzeba uzupełnić
    }
};
