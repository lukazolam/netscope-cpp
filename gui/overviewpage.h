
#pragma once

#include <QWidget>

#include <vector>

#include "systeminfo.h"

class QTableWidget;
class QPlainTextEdit;
class QCheckBox;
class QLabel;

class OverviewPage : public QWidget {
public:
    explicit OverviewPage(QWidget* parent = nullptr);

private:
    void refreshInterfaces();
    void updateDetails(int row);
    void updateSummary();

    QTableWidget* interfaceTable_ = nullptr;
    QPlainTextEdit* details_ = nullptr;
    QCheckBox* showInactive_ = nullptr;

    QLabel* totalLabel_ = nullptr;
    QLabel* activeLabel_ = nullptr;
    QLabel* ipv4Label_ = nullptr;

    std::vector<NetworkInterface> interfaces_;
    std::vector<std::string> systemDns_;
};
