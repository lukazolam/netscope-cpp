
#pragma once

#include <QWidget>

#include <functional>

#include "settings.h"

class QSpinBox;
class QCheckBox;
class QLabel;
class QPushButton;

class SettingsPage : public QWidget {
public:
    explicit SettingsPage(
        AppSettings& settings,
        std::function<void()> onSettingsSaved,
        QWidget* parent = nullptr
    );

private:
    void loadForm();
    void saveSettings();
    void restoreDefaults();

    AppSettings& settings_;
    std::function<void()> onSettingsSaved_;

    QSpinBox* threadsInput_ = nullptr;
    QSpinBox* tcpTimeoutInput_ = nullptr;
    QSpinBox* icmpTimeoutInput_ = nullptr;

    QCheckBox* icmpCheck_ = nullptr;
    QCheckBox* arpCheck_ = nullptr;
    QCheckBox* tcpCheck_ = nullptr;

    QLabel* statusLabel_ = nullptr;

    QPushButton* saveButton_ = nullptr;
    QPushButton* defaultsButton_ = nullptr;
};
