#include "../../include/service/application_service.h"

#include <qstring.h>

#include "../../include/repository/signal_repository.h"

class ApplicationService : public IApplicationService {
    std::shared_ptr<ISignalRepository> signalRepo_;

public:
    explicit ApplicationService(std::shared_ptr<ISignalRepository> repo)
        : signalRepo_(std::move(repo)) {
    }

    bool Load(QString filename) override{}

    int GetLength() override{}

    SignalRange GetViewRange() override{}

    void SetViewRange(SignalRange range) override{}

    std::shared_ptr<std::vector<SignalDatapoint>> GetData() override{}

    std::shared_ptr<std::vector<SignalDatapoint>> GetFilteredData() override{}

    Status GetStatus() override{}

    bool RunFiltering(FilterMethod method) override{}
};
