#ifndef ROOT_H
#define ROOT_H

#include <QMainWindow>
#include <memory>

#include "include/service/abstract/application_service.h"


class IApplicationService;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(std::shared_ptr<IApplicationService> application_service,
                        QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onLoadDataClicked();

private:
    std::shared_ptr<IApplicationService> application_service_;
    Ui::MainWindow *ui;
};

#endif // ROOT_H
