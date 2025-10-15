#include "../../include/service/hrv_dfa_processing_service.h"

class HRVDFAProcessingService :public IHRVDFAProcessingService {
public:
    HRVDFAMetrics Process(std::vector<SignalDatapoint> datapoints, int frequency) override {
        // TODO(Hubert): trzeba uzupełnić
    }
};
