#ifndef ROOT_H
#define ROOT_H

#include <QMainWindow>

#include "include/service/application_service.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

    std::shared_ptr<IApplicationService> application_service_;

public:
    MainWindow(std::shared_ptr<IApplicationService> application_service, QWidget *parent = nullptr);

    ~MainWindow();
};
#endif // ROOT_H
