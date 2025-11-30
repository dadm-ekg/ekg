#include "../../include/bridge/ekg_controller.h"
#include "../../include/dto/filter_method.h"
#include "../../include/dto/r_peaks_detection_method.h"
#include <QFileDialog>
#include <QCoreApplication>
#include <QDir>

EkgController::EkgController(std::shared_ptr<IApplicationService> application_service, QObject *parent)
    : QObject(parent), application_service_(std::move(application_service)) {
}

void EkgController::loadData(const QString &filename) {
    if (filename.isEmpty()) {
        emit fileLoadError("Nie wybrano pliku");
        return;
    }

    bool success = application_service_->Load(filename);

    if (success) {
        emit loadedFilenameChanged();
        emit isFileLoadedChanged();
        emit fileLoadSuccess(filename);
    } else {
        emit fileLoadError("Nie udało się załadować pliku");
    }
}

void EkgController::openFileDialog() {
    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);

    while (!dir.exists("ludb") && dir.cdUp()) {
    }

    QString ludbPath = dir.absoluteFilePath("ludb");

    QString filename = QFileDialog::getOpenFileName(
        nullptr,
        "Wybierz plik danych EKG",
        ludbPath,
        "DAT Files (*.dat);;All Files (*)"
    );

    if (!filename.isEmpty()) {
        loadData(filename);
    }
}

bool EkgController::runFiltering(int method) {
    return application_service_->RunFiltering(static_cast<FilterMethod>(method));
}

bool EkgController::calculateRPeaks(int method) {
    return application_service_->CalculateRPeaks(static_cast<RPeaksDetectionMethod>(method));
}

QString EkgController::loadedFilename() const {
    return application_service_->GetLoadedFilename();
}

bool EkgController::isFileLoaded() const {
    return application_service_->IsFileLoaded();
}

