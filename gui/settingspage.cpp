
#include "settingspage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QFont>
#include <QSettings>
#include <QString>

#include <utility>

SettingsPage::SettingsPage(
    AppSettings& settings,
    std::function<void()> onSettingsSaved,
    QWidget* parent
)
    : QWidget(parent),
      settings_(settings),
      onSettingsSaved_(std::move(onSettingsSaved)) {

    auto* layout = new QVBoxLayout(this);

    // Page heading
    auto* title = new QLabel("Settings");

    QFont font = title->font();
    font.setPointSize(20);
    font.setBold(true);
    title->setFont(font);

    layout->addWidget(title);

    auto* description = new QLabel(
        "Configure scanner performance and "
        "default network discovery methods."
    );

    description->setWordWrap(true);
    layout->addWidget(description);

    // Scanner performance section
    auto* performanceGroup =
        new QGroupBox("Scanner Performance");

    auto* performanceLayout =
        new QFormLayout(performanceGroup);

    threadsInput_ = new QSpinBox();
    threadsInput_->setRange(1, 256);
    threadsInput_->setSuffix(" threads");

    tcpTimeoutInput_ = new QSpinBox();
    tcpTimeoutInput_->setRange(100, 10000);
    tcpTimeoutInput_->setSingleStep(100);
    tcpTimeoutInput_->setSuffix(" ms");

    icmpTimeoutInput_ = new QSpinBox();
    icmpTimeoutInput_->setRange(100, 10000);
    icmpTimeoutInput_->setSingleStep(100);
    icmpTimeoutInput_->setSuffix(" ms");

    performanceLayout->addRow(
        "Maximum concurrent workers:",
        threadsInput_
    );

    performanceLayout->addRow(
        "TCP connection timeout:",
        tcpTimeoutInput_
    );

    performanceLayout->addRow(
        "ICMP response timeout:",
        icmpTimeoutInput_
    );

    layout->addWidget(performanceGroup);

    // Network discovery section
    auto* discoveryGroup =
        new QGroupBox("Default Discovery Methods");

    auto* discoveryLayout =
        new QVBoxLayout(discoveryGroup);

    auto* discoveryDescription = new QLabel(
        "These defaults are used for new discovery scans. "
        "You can override them in Network Discovery."
    );

    discoveryDescription->setWordWrap(true);
    discoveryLayout->addWidget(discoveryDescription);

    icmpCheck_ = new QCheckBox("Enable ICMP");
    arpCheck_ = new QCheckBox("Enable ARP");
    tcpCheck_ = new QCheckBox("Enable TCP");

    discoveryLayout->addWidget(icmpCheck_);
    discoveryLayout->addWidget(arpCheck_);
    discoveryLayout->addWidget(tcpCheck_);

    layout->addWidget(discoveryGroup);

    // Status and action buttons
    statusLabel_ = new QLabel(
        "Changes apply to new scans after saving."
    );

    layout->addWidget(statusLabel_);

    auto* buttonLayout = new QHBoxLayout();

    defaultsButton_ =
        new QPushButton("Restore Defaults");

    saveButton_ =
        new QPushButton("Save Settings");

    buttonLayout->addWidget(defaultsButton_);
    buttonLayout->addStretch();
    buttonLayout->addWidget(saveButton_);

    layout->addLayout(buttonLayout);
    layout->addStretch();

    connect(
        saveButton_,
        &QPushButton::clicked,
        this,
        [this]() {
            saveSettings();
        }
    );

    connect(
        defaultsButton_,
        &QPushButton::clicked,
        this,
        [this]() {
            restoreDefaults();
        }
    );

    // Populate the controls from current settings
    loadForm();
}

void SettingsPage::loadForm() {
    threadsInput_->setValue(
        static_cast<int>(settings_.maxThreads)
    );

    tcpTimeoutInput_->setValue(
        static_cast<int>(settings_.tcpTimeoutMs)
    );

    icmpTimeoutInput_->setValue(
        static_cast<int>(settings_.icmpTimeoutMs)
    );

    icmpCheck_->setChecked(settings_.enableICMP);
    arpCheck_->setChecked(settings_.enableARP);
    tcpCheck_->setChecked(settings_.enableTCP);
}

void SettingsPage::saveSettings() {
    // Read the controls into a temporary settings object.
    AppSettings updated = settings_;

    updated.maxThreads =
        static_cast<unsigned int>(threadsInput_->value());

    updated.tcpTimeoutMs =
        static_cast<unsigned int>(tcpTimeoutInput_->value());

    updated.icmpTimeoutMs =
        static_cast<unsigned int>(icmpTimeoutInput_->value());

    updated.enableICMP = icmpCheck_->isChecked();
    updated.enableARP = arpCheck_->isChecked();
    updated.enableTCP = tcpCheck_->isChecked();

    // Persist the values to the operating system.
    QSettings storage("NetScope", "NetScope");

    storage.setValue(
        "scanner/maxThreads",
        static_cast<int>(updated.maxThreads)
    );

    storage.setValue(
        "scanner/tcpTimeoutMs",
        static_cast<int>(updated.tcpTimeoutMs)
    );

    storage.setValue(
        "scanner/icmpTimeoutMs",
        static_cast<int>(updated.icmpTimeoutMs)
    );

    storage.setValue(
        "discovery/enableICMP",
        updated.enableICMP
    );

    storage.setValue(
        "discovery/enableARP",
        updated.enableARP
    );

    storage.setValue(
        "discovery/enableTCP",
        updated.enableTCP
    );

    storage.sync();

    if (storage.status() != QSettings::NoError) {
        statusLabel_->setText(
            "Error: Could not save settings."
        );
        return;
    }

    // Only publish the new values after saving succeeded.
    settings_ = updated;

    if (onSettingsSaved_) {
        onSettingsSaved_();
    }

    statusLabel_->setText(
        "Settings saved successfully."
    );
}

void SettingsPage::restoreDefaults() {
    // Reset the controls, but don't persist until Save.
    const AppSettings defaults{};

    threadsInput_->setValue(
        static_cast<int>(defaults.maxThreads)
    );

    tcpTimeoutInput_->setValue(
        static_cast<int>(defaults.tcpTimeoutMs)
    );

    icmpTimeoutInput_->setValue(
        static_cast<int>(defaults.icmpTimeoutMs)
    );

    icmpCheck_->setChecked(defaults.enableICMP);
    arpCheck_->setChecked(defaults.enableARP);
    tcpCheck_->setChecked(defaults.enableTCP);

    statusLabel_->setText(
        "Defaults restored. Click Save Settings to apply."
    );
}
