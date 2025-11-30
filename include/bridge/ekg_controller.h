#ifndef EKG_CONTROLLER_H
#define EKG_CONTROLLER_H

#include <QObject>
#include <QString>
#include <memory>
#include "../service/abstract/application_service.h"

class EkgController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString loadedFilename READ loadedFilename NOTIFY loadedFilenameChanged)
    Q_PROPERTY(bool isFileLoaded READ isFileLoaded NOTIFY isFileLoadedChanged)
    Q_PROPERTY(bool hasData READ hasData NOTIFY hasDataChanged)
    Q_PROPERTY(bool hasFilteredData READ hasFilteredData NOTIFY hasFilteredDataChanged)
    Q_PROPERTY(bool baselineCompleted READ baselineCompleted NOTIFY baselineCompletedChanged)
    Q_PROPERTY(bool rPeaksCompleted READ rPeaksCompleted NOTIFY rPeaksCompletedChanged)

public:
    enum FilterMethod {
        MovingAverage = 0,
        Butterworth = 1,
        SavitzkyGolay = 2
    };
    Q_ENUM(FilterMethod)

    enum RPeaksMethod {
        PanTompkins = 0,
        Hilbert = 1,
        Wavelet = 2
    };
    Q_ENUM(RPeaksMethod)

    explicit EkgController(std::shared_ptr<IApplicationService> application_service, QObject *parent = nullptr);

    Q_INVOKABLE void loadData(const QString &filename);
    Q_INVOKABLE void openFileDialog();
    Q_INVOKABLE bool runBaseline(int filterMethod);
    Q_INVOKABLE bool runRPeaksDetection(int method);
    Q_INVOKABLE QStringList getAvailableFiles() const;
    Q_INVOKABLE void loadFileByName(const QString &filename);

    QString loadedFilename() const;
    bool isFileLoaded() const;
    bool hasData() const;
    bool hasFilteredData() const;
    bool baselineCompleted() const;
    bool rPeaksCompleted() const;

signals:
    void loadedFilenameChanged();
    void isFileLoadedChanged();
    void hasDataChanged();
    void hasFilteredDataChanged();
    void baselineCompletedChanged();
    void rPeaksCompletedChanged();
    void fileLoadSuccess(const QString &filename);
    void fileLoadError(const QString &errorMessage);
    void filteringSuccess(const QString &filterName);
    void filteringError(const QString &errorMessage);
    void rPeaksDetectionSuccess(const QString &methodName);
    void rPeaksDetectionError(const QString &errorMessage);

private:
    std::shared_ptr<IApplicationService> application_service_;
    bool baseline_completed_ = false;
    bool r_peaks_completed_ = false;
};

#endif

