
#include "overviewpage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QPlainTextEdit>
#include <QFont>
#include <QVariant>
#include <QStringList>

#include <string>

namespace {

// Convert a C++ string to a Qt string
QString toQString(const std::string& value) {
    return QString::fromStdString(value);
}

// Display a fallback when a value is unavailable
QString displayValue(const std::string& value) {
    return value.empty()
        ? QString("Not available")
        : toQString(value);
}

// Join several strings for display
QString joinStrings(
    const std::vector<std::string>& values
) {
    QStringList items;

    for (const auto& value : values) {
        items.append(toQString(value));
    }

    return items.isEmpty()
        ? QString("Not available")
        : items.join(", ");
}

} // namespace

OverviewPage::OverviewPage(QWidget* parent)
    : QWidget(parent) {

    auto* layout = new QVBoxLayout(this);

    // Page heading
    auto* headingLayout = new QHBoxLayout();

    auto* title = new QLabel("Network Overview");

    QFont titleFont = title->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    title->setFont(titleFont);

    auto* refreshButton = new QPushButton("Refresh");

    headingLayout->addWidget(title);
    headingLayout->addStretch();
    headingLayout->addWidget(refreshButton);

    layout->addLayout(headingLayout);

    // Summary statistics
    auto* summaryLayout = new QHBoxLayout();

    totalLabel_ = new QLabel();
    activeLabel_ = new QLabel();
    ipv4Label_ = new QLabel();

    summaryLayout->addWidget(totalLabel_);
    summaryLayout->addWidget(activeLabel_);
    summaryLayout->addWidget(ipv4Label_);

    layout->addLayout(summaryLayout);

    // Interface display filter
    showInactive_ = new QCheckBox(
        "Show inactive interfaces"
    );

    showInactive_->setChecked(false);
    layout->addWidget(showInactive_);

    // Interface table
    interfaceTable_ = new QTableWidget();

    interfaceTable_->setColumnCount(5);
    interfaceTable_->setHorizontalHeaderLabels({
        "Interface",
        "Status",
        "IPv4 Address",
        "MAC Address",
        "MTU"
    });

    interfaceTable_->horizontalHeader()
        ->setSectionResizeMode(QHeaderView::Stretch);

    interfaceTable_->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );

    interfaceTable_->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );

    interfaceTable_->setSelectionMode(
        QAbstractItemView::SingleSelection
    );

    interfaceTable_->setSortingEnabled(true);

    layout->addWidget(interfaceTable_, 2);

    // Detailed adapter information
    layout->addWidget(
        new QLabel("Selected Interface Details")
    );

    details_ = new QPlainTextEdit();
    details_->setReadOnly(true);
    details_->setPlaceholderText(
        "Select a network interface to view its details."
    );

    layout->addWidget(details_, 1);

    // Connect refresh action
    connect(
        refreshButton,
        &QPushButton::clicked,
        this,
        [this]() {
            refreshInterfaces();
        }
    );

    // Update display when filter changes
    connect(
        showInactive_,
        &QCheckBox::toggled,
        this,
        [this](bool) {
            refreshInterfaces();
        }
    );

    // Show details when selecting an interface
    connect(
        interfaceTable_,
        &QTableWidget::currentCellChanged,
        this,
        [this](
            int currentRow,
            int,
            int,
            int
        ) {
            updateDetails(currentRow);
        }
    );

    // Initial data load
    refreshInterfaces();
}

void OverviewPage::refreshInterfaces() {
    // Collect the current operating system data
    interfaces_ = getNetworkInterfaces();
    systemDns_ = getSystemDNSServers();

    updateSummary();

    interfaceTable_->setSortingEnabled(false);
    interfaceTable_->setRowCount(0);

    for (std::size_t index = 0;
         index < interfaces_.size();
         ++index) {

        const NetworkInterface& adapter =
            interfaces_[index];

        if (!showInactive_->isChecked() &&
            !adapter.isUp) {
            continue;
        }

        QStringList ipv4Addresses;

        for (const auto& address : adapter.addresses) {
            if (address.family == 4) {
                ipv4Addresses.append(
                    toQString(address.address)
                );
            }
        }

        int row = interfaceTable_->rowCount();
        interfaceTable_->insertRow(row);

        // Preserve the original vector index for
        // correct detail selection after sorting
        auto* nameItem = new QTableWidgetItem(
            toQString(adapter.name)
        );

        nameItem->setData(
            Qt::UserRole,
            static_cast<int>(index)
        );

        interfaceTable_->setItem(row, 0, nameItem);

        interfaceTable_->setItem(
            row, 1,
            new QTableWidgetItem(
                adapter.isUp ? "Up" : "Down"
            )
        );

        interfaceTable_->setItem(
            row, 2,
            new QTableWidgetItem(
                ipv4Addresses.isEmpty()
                    ? "None"
                    : ipv4Addresses.join(", ")
            )
        );

        interfaceTable_->setItem(
            row, 3,
            new QTableWidgetItem(
                displayValue(adapter.macAddress)
            )
        );

        auto* mtuItem = new QTableWidgetItem();
        mtuItem->setData(
            Qt::EditRole,
            static_cast<int>(adapter.mtu)
        );

        interfaceTable_->setItem(row, 4, mtuItem);
    }

    interfaceTable_->setSortingEnabled(true);

    if (interfaceTable_->rowCount() > 0) {
        interfaceTable_->selectRow(0);
        updateDetails(0);
    } else {
        details_->setPlainText(
            "No network interfaces match the current filter."
        );
    }
}

void OverviewPage::updateSummary() {
    int activeCount = 0;
    int ipv4Count = 0;

    for (const auto& adapter : interfaces_) {
        if (adapter.isUp) {
            ++activeCount;
        }

        for (const auto& address : adapter.addresses) {
            if (address.family == 4) {
                ++ipv4Count;
            }
        }
    }

    totalLabel_->setText(
        QString("Total Interfaces: %1")
            .arg(static_cast<int>(interfaces_.size()))
    );

    activeLabel_->setText(
        QString("Active: %1").arg(activeCount)
    );

    ipv4Label_->setText(
        QString("IPv4 Addresses: %1").arg(ipv4Count)
    );
}

void OverviewPage::updateDetails(int row) {
    if (row < 0 ||
        row >= interfaceTable_->rowCount()) {
        details_->clear();
        return;
    }

    auto* item = interfaceTable_->item(row, 0);

    if (!item) {
        return;
    }

    int index = item->data(Qt::UserRole).toInt();

    if (index < 0 ||
        static_cast<std::size_t>(index) >=
            interfaces_.size()) {
        return;
    }

    const NetworkInterface& adapter =
        interfaces_[index];

    QString text;

    text += "Name: " + toQString(adapter.name) + "\n";
    text += QString("Status: %1\n")
        .arg(adapter.isUp ? "Up" : "Down");

    text += "MAC Address: " +
        displayValue(adapter.macAddress) + "\n";

    text += QString("MTU: %1\n")
        .arg(adapter.mtu);

    text += "Default IPv4 Gateway: " +
        displayValue(adapter.defaultGateway) + "\n";

    text += "\nIP Addresses:\n";

    if (adapter.addresses.empty()) {
        text += "  None\n";
    }

    for (const auto& address : adapter.addresses) {
        text += QString("  IPv%1: %2/%3\n")
            .arg(address.family)
            .arg(toQString(address.address))
            .arg(address.prefixLength);

        if (address.family == 4 &&
            !address.subnetMask.empty()) {
            text += "    Subnet Mask: " +
                toQString(address.subnetMask) + "\n";
        }
    }

    text += "\nDNS Servers:\n";

    if (!adapter.dnsServers.empty()) {
        for (const auto& server : adapter.dnsServers) {
            text += "  " + toQString(server) + "\n";
        }
    }
    else if (!systemDns_.empty()) {
        text += "  System resolver information:\n";

        for (const auto& server : systemDns_) {
            text += "  " + toQString(server) + "\n";
        }
    }
    else {
        text += "  Not available\n";
    }

    details_->setPlainText(text);
}
