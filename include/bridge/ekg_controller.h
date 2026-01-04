#ifndef EKG_CONTROLLER_H
#define EKG_CONTROLLER_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
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
    Q_PROPERTY(bool hrvTimeCompleted READ hrvTimeCompleted NOTIFY hrvTimeCompletedChanged)
    Q_PROPERTY(bool hrvGeoCompleted READ hrvGeoCompleted NOTIFY hrvGeoCompletedChanged)
    Q_PROPERTY(bool wavesCompleted READ wavesCompleted NOTIFY wavesCompletedChanged)

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

    enum HRVSpectralMethod {
        ClassicPeriodogram = 0,
        LombScargle = 1,
        Welch = 2
    };
    Q_ENUM(HRVSpectralMethod)

    explicit EkgController(std::shared_ptr<IApplicationService> application_service, QObject *parent = nullptr);

    Q_INVOKABLE void loadData(const QString &filename);
    Q_INVOKABLE void openFileDialog();
    Q_INVOKABLE bool runBaseline(int filterMethod);
    Q_INVOKABLE bool runRPeaksDetection(int method);
    Q_INVOKABLE bool runHRVTime(int method);
    Q_INVOKABLE bool runHRVGeo();
    Q_INVOKABLE bool runWaves();
    Q_INVOKABLE QStringList getAvailableFiles() const;
    Q_INVOKABLE void loadFileByName(const QString &filename);
    Q_INVOKABLE void resetBaseline();
    Q_INVOKABLE void resetRPeaks();
    Q_INVOKABLE void resetHRVTime();
    Q_INVOKABLE void resetHRVGeo();
    Q_INVOKABLE void resetWaves();
    Q_INVOKABLE QVariantList getRawSeries(int channel = 0, int maxPoints = 4000) const;
    Q_INVOKABLE QVariantList getFilteredSeries(int channel = 0, int maxPoints = 4000) const;
    Q_INVOKABLE QVariantList getRPeakMarkers(int channel = 0) const;
    Q_INVOKABLE QVariantMap getHRVTimeMetrics() const;
    Q_INVOKABLE QVariantMap getHRVGeoMetrics() const;
    Q_INVOKABLE QVariantList getHRVGeoHistogram() const;
    Q_INVOKABLE QVariantList getHRVGeoPoincare() const;
    Q_INVOKABLE QVariantMap getWaveMarkers(int channel = 0) const;
    Q_INVOKABLE int channelCount() const;
    Q_INVOKABLE double samplingFrequency() const;

    QString loadedFilename() const;
    bool isFileLoaded() const;
    bool hasData() const;
    bool hasFilteredData() const;
    bool baselineCompleted() const;
    bool rPeaksCompleted() const;
    bool hrvTimeCompleted() const;
    bool hrvGeoCompleted() const;
    bool wavesCompleted() const;

signals:
    void loadedFilenameChanged();
    void isFileLoadedChanged();
    void hasDataChanged();
    void hasFilteredDataChanged();
    void baselineCompletedChanged();
    void rPeaksCompletedChanged();
    void hrvTimeCompletedChanged();
    void hrvGeoCompletedChanged();
    void wavesCompletedChanged();
    void fileLoadSuccess(const QString &filename);
    void fileLoadError(const QString &errorMessage);
    void filteringSuccess(const QString &filterName);
    void filteringError(const QString &errorMessage);
    void rPeaksDetectionSuccess(const QString &methodName);
    void rPeaksDetectionError(const QString &errorMessage);
    void hrvTimeSuccess(const QString &methodName);
    void hrvTimeError(const QString &errorMessage);
    void hrvGeoSuccess();
    void hrvGeoError(const QString &errorMessage);
    void wavesSuccess();
    void wavesError(const QString &errorMessage);

private:
    std::shared_ptr<IApplicationService> application_service_;
    bool baseline_completed_ = false;
    bool r_peaks_completed_ = false;
    bool hrv_time_completed_ = false;
    bool hrv_geo_completed_ = false;
    bool waves_completed_ = false;
    HRVTimeMetrics cached_hrv_metrics_;
    HRVGeoMetrics cached_hrv_geo_metrics_;
};

#endif
