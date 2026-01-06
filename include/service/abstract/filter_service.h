#ifndef EKG_FILTER_SERVICE_H
#define EKG_FILTER_SERVICE_H
#include <vector>

#include "../../model/signal_datapoint.h"

class IFilterService {
public:
    virtual ~IFilterService() = default;

    virtual std::vector<SignalDatapoint> Filter(const std::vector<SignalDatapoint>& values, int windowSize = 5, int polynomialOrder = 2) = 0;
};

#endif //EKG_FILTER_SERVICE_H
