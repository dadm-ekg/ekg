#include "../../include/repository/csv_signal_repository.h"
#include <iostream>
#include <ostream>

std::shared_ptr<SignalDataset> CSVSignalRepository::Load(const QString& filename) {
    // TODO(Paulina): trzeba uzupełnić
    std::cout << "CSVSignalRepository::Load " << filename.toStdString() << std::endl;
    return std::make_shared<SignalDataset>();
}
