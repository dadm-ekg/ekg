#ifndef EKG_WAVE_ANNOTATED_SIGNAL_DATAPOINT_H
#define EKG_WAVE_ANNOTATED_SIGNAL_DATAPOINT_H
#include "signal_datapoint.h"

class WaveAnnotatedSignalDatapoint : SignalDatapoint {
// TODO(Magda): trzeba zweryfikować
public:
    bool p_wave;
    bool q_wave;
    bool rs_wave;
    bool t_wave;
};

#endif //EKG_WAVE_ANNOTATED_SIGNAL_DATAPOINT_H
