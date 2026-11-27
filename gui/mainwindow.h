
#pragma once

#include <QMainWindow>

#include "settings.h"

class QStackedWidget;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    AppSettings settings_;
    QStackedWidget* pages_ = nullptr;

    void setupInterface();
};
