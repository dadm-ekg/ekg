#ifndef EKG_APPLICATION_SERVICE_H
#define EKG_APPLICATION_SERVICE_H
#include <qstring.h>

#include "../dto/signal_range.h"
#include "../model/signal_datapoint.h"

class IApplicationService {
public:
    virtual ~IApplicationService() = default;

    virtual bool Load(QString filename) = 0;

    void SetFrequency(double frequency);

    double GetFrequency();

    int GetLength();

    SignalRange GetPreviewRange();

    void SetPreviewRange(SignalRange range);

    std::vector<SignalDatapoint> GetPreviewData();
};

#endif //EKG_APPLICATION_SERVICE_H
