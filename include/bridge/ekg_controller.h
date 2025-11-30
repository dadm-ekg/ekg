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

public:
    enum FilterMethod {
        MovingAverage = 0,
        Butterworth = 1,
        SavitzkyGolay = 2
    };
    Q_ENUM(FilterMethod)

    explicit EkgController(std::shared_ptr<IApplicationService> application_service, QObject *parent = nullptr);

    Q_INVOKABLE void loadData(const QString &filename);
    Q_INVOKABLE void openFileDialog();
    Q_INVOKABLE bool runBaseline(int filterMethod);
    Q_INVOKABLE bool calculateRPeaks(int method);

    QString loadedFilename() const;
    bool isFileLoaded() const;
    bool hasData() const;

signals:
    void loadedFilenameChanged();
    void isFileLoadedChanged();
    void hasDataChanged();
    void fileLoadSuccess(const QString &filename);
    void fileLoadError(const QString &errorMessage);
    void filteringSuccess(const QString &filterName);
    void filteringError(const QString &errorMessage);

private:
    std::shared_ptr<IApplicationService> application_service_;
};

#endif

