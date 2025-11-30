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

public:
    explicit EkgController(std::shared_ptr<IApplicationService> application_service, QObject *parent = nullptr);

    Q_INVOKABLE void loadData(const QString &filename);
    Q_INVOKABLE void openFileDialog();
    Q_INVOKABLE bool runFiltering(int method);
    Q_INVOKABLE bool calculateRPeaks(int method);

    QString loadedFilename() const;
    bool isFileLoaded() const;

signals:
    void loadedFilenameChanged();
    void isFileLoadedChanged();
    void fileLoadSuccess(const QString &filename);
    void fileLoadError(const QString &errorMessage);

private:
    std::shared_ptr<IApplicationService> application_service_;
};

#endif

