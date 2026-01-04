#ifndef EKG_WAVE_ANNOTATED_SIGNAL_DATAPOINT_H
#define EKG_WAVE_ANNOTATED_SIGNAL_DATAPOINT_H
#include "signal_datapoint.h"

class WaveAnnotatedSignalDatapoint : public SignalDatapoint {
public:
    bool p_wave_onset = false;
    bool p_wave_end = false;
    bool qrs_onset = false;
    bool qrs_end = false;
    bool t_end = false;
};

#endif //EKG_WAVE_ANNOTATED_SIGNAL_DATAPOINT_H
