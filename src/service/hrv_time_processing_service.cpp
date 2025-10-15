#include "../../include/service/hrv_time_processing_service.h"

class HRVTimeProcessingService : public IHRVTimeProcessingService {
public:
    HRVTimeMetrics Process(std::vector<SignalDatapoint> datapoints, int frequency) override {
        // TODO(Kuba Nowak): trzeba uzupełnić
    }
};
