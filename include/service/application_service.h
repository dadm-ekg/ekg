#ifndef EKG_APPLICATION_SERVICE_H
#define EKG_APPLICATION_SERVICE_H

#include "../dto/signal_range.h"
#include "../dto/status.h"
#include "../model/signal_datapoint.h"

class IApplicationService {
public:
    virtual ~IApplicationService() = default;

    virtual bool Load(QString filename) = 0;

    virtual void SetFrequency(double frequency) = 0 ;

    virtual double GetFrequency()= 0 ;

    virtual int GetLength()= 0 ;

    virtual SignalRange GetPreviewRange()= 0 ;

    virtual void SetPreviewRange(SignalRange range)= 0 ;

    virtual std::vector<SignalDatapoint> GetPreviewData()= 0 ;

    virtual Status GetStatus()= 0 ;
};

#endif //EKG_APPLICATION_SERVICE_H
