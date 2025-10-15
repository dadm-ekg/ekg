#ifndef EKG_APPLICATION_SERVICE_H
#define EKG_APPLICATION_SERVICE_H

#include "../dto/signal_range.h"
#include "../dto/status.h"
#include "../dto/filter_method.h"
#include "../model/signal_datapoint.h"

class IApplicationService {
public:
    virtual ~IApplicationService() = default;

    virtual bool Load(QString filename) = 0;

    virtual int GetLength() = 0;

    virtual SignalRange GetViewRange() = 0;

    virtual void SetViewRange(SignalRange range) = 0;

    virtual std::shared_ptr<std::vector<SignalDatapoint>> GetData() = 0;
    virtual std::shared_ptr<std::vector<SignalDatapoint>> GetFilteredData() = 0;

    virtual Status GetStatus() = 0;

    virtual bool RunFiltering(FilterMethod method) = 0;
};

#endif //EKG_APPLICATION_SERVICE_H
