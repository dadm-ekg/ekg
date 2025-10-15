#include "../../include/service/application_service.h"

#include <qstring.h>

#include "../../include/repository/signal_repository.h"

class ApplicationService : public IApplicationService {
    std::shared_ptr<ISignalRepository> signalRepo_;

public:
    explicit ApplicationService(std::shared_ptr<ISignalRepository> repo)
        : signalRepo_(std::move(repo)) {
    }

    bool Load(QString filename) override {
        signalRepo_->Load(filename);
    }

    void SetFrequency(double frequency) override {
    }

    double GetFrequency() override {
    }

    int GetLength() override {
    }

    SignalRange GetPreviewRange() override {
    }

    void SetPreviewRange(SignalRange range) override {
    }

    std::vector<SignalDatapoint> GetPreviewData() override {
    }

    Status GetStatus() override {
    }
};
