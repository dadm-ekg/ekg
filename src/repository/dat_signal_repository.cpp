#include "../../include/repository/dat_signal_repository.h"
#include <iostream>
#include <ostream>

std::shared_ptr<SignalDataset> DATSignalRepository::Load(const QString& filename) {
    // TODO(Paulina): trzeba uzupełnić
    std::cout << "DATSignalRepository::Load " << filename.toStdString() << std::endl;
    return std::make_shared<SignalDataset>();
}
