#ifndef EKG_R_PEAKS_ANNOTATED_SIGNAL_DATAPOINT_H
#define EKG_R_PEAKS_ANNOTATED_SIGNAL_DATAPOINT_H
#include "signal_datapoint.h"

class RPeaksAnnotatedSignalDatapoint : public SignalDatapoint {
public:
    bool peak;
};

#endif //EKG_R_PEAKS_ANNOTATED_SIGNAL_DATAPOINT_H
