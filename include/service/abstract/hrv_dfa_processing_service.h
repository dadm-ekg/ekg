#ifndef EKG_HRV_DFA_PROCESSING_SERVICE_H
#define EKG_HRV_DFA_PROCESSING_SERVICE_H
#include <vector>

#include "../../dto/hrv_dfa_metrics.h"
#include "../../model/signal_datapoint.h"

class IHRVDFAProcessingService {
public:
    virtual ~IHRVDFAProcessingService() = default;

    virtual HRVDFAMetrics Process(const std::vector<SignalDatapoint>& datapoints, int frequency) = 0;
};

#endif //EKG_HRV_DFA_PROCESSING_SERVICE_H

