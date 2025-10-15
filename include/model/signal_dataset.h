#ifndef EKG_SIGNAL_DATASET_H
#define EKG_SIGNAL_DATASET_H
#include <vector>

#include "signal_datapoint.h"

class SignalDataset {
public:
    std::vector<SignalDatapoint> values;
    int frequency;
};

#endif //EKG_SIGNAL_DATASET_H
