#include "../../include/repository/signal_repository.h"

class CSVSignalRepository : public ISignalRepository {
public:
    std::shared_ptr<std::vector<SignalDatapoint> > Load(QString filename) override {
        return std::make_shared<std::vector<SignalDatapoint> >();
    }
};
