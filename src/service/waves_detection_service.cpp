#include "../../include/service/waves_detection_service.h"

class WavesDetectionService : public IWavesDetectionService {
public:
    std::vector<WaveAnnotatedSignalDatapoint> Detect(std::vector<SignalDatapoint> datapoints, int frequency) override {
        // TODO(Magda): trzeba uzupełnić
    }
};
