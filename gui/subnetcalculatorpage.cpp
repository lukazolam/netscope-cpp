
#include "subnetcalculatorpage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QFont>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

#include "network.h"

SubnetCalculatorPage::SubnetCalculatorPage(
    QWidget* parent
)
    : QWidget(parent) {

    auto* layout = new QVBoxLayout(this);

    // Page heading
    auto* title = new QLabel("IPv4 Subnet Calculator");

    QFont font = title->font();
    font.setPointSize(20);
    font.setBold(true);
    title->setFont(font);

    layout->addWidget(title);

    // Short explanation
    auto* description = new QLabel(
        "Calculate network addresses, subnet masks, "
        "broadcast addresses and usable host ranges."
    );

    description->setWordWrap(true);
    layout->addWidget(description);

    // Input controls
    auto* inputLayout = new QHBoxLayout();

    ipInput_ = new QLineEdit();
    ipInput_->setPlaceholderText(
        "IPv4 address (e.g. 172.16.0.1)"
    );

    ipInput_->setText("192.168.0.1");

    // Restrict input to dotted-decimal characters
    QRegularExpression expression(
        R"([0-9.]{0,15})"
    );

    ipInput_->setValidator(
        new QRegularExpressionValidator(
            expression, ipInput_
        )
    );

    prefixInput_ = new QSpinBox();
    prefixInput_->setRange(0, 32);
    prefixInput_->setValue(24);
    prefixInput_->setPrefix("/");

    inputLayout->addWidget(new QLabel("IPv4:"));
    inputLayout->addWidget(ipInput_, 1);

    inputLayout->addWidget(new QLabel("Prefix:"));
    inputLayout->addWidget(prefixInput_);

    layout->addLayout(inputLayout);

    // Network summary
    networkLabel_ = new QLabel();

    QFont networkFont = networkLabel_->font();
    networkFont.setPointSize(16);
    networkFont.setBold(true);
    networkLabel_->setFont(networkFont);

    layout->addWidget(networkLabel_);

    // Validation / status information
    statusLabel_ = new QLabel();
    layout->addWidget(statusLabel_);

    // Results table
    results_ = new QTableWidget();

    results_->setColumnCount(2);
    results_->setRowCount(6);

    results_->setHorizontalHeaderLabels({
        "Property", "Value"
    });

    results_->horizontalHeader()
        ->setSectionResizeMode(QHeaderView::Stretch);

    results_->verticalHeader()->setVisible(false);

    results_->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );

    results_->setSelectionMode(
        QAbstractItemView::NoSelection
    );

    const QStringList properties = {
        "Subnet Mask",
        "Network Address",
        "Broadcast Address",
        "First Usable Host",
        "Last Usable Host",
        "Usable Host Addresses"
    };

    for (int row = 0; row < properties.size(); ++row) {
        results_->setItem(
            row, 0,
            new QTableWidgetItem(properties[row])
        );

        results_->setItem(
            row, 1,
            new QTableWidgetItem("-")
        );
    }

    layout->addWidget(results_);

    // Action buttons
    auto* buttonLayout = new QHBoxLayout();

    copyButton_ = new QPushButton("Copy Results");

    buttonLayout->addStretch();
    buttonLayout->addWidget(copyButton_);

    layout->addLayout(buttonLayout);

    // Automatically recalculate when IP changes
    connect(
        ipInput_,
        &QLineEdit::textChanged,
        this,
        [this](const QString&) {
            calculate();
        }
    );

    // Automatically recalculate when prefix changes
    connect(
        prefixInput_,
        QOverload<int>::of(&QSpinBox::valueChanged),
        this,
        [this](int) {
            calculate();
        }
    );

    connect(
        copyButton_,
        &QPushButton::clicked,
        this,
        [this]() {
            copyResults();
        }
    );

    // Calculate initial example network
    calculate();
}

void SubnetCalculatorPage::calculate() {
    const std::string ip =
        ipInput_->text().trimmed().toStdString();

    const int prefix = prefixInput_->value();

    if (!validateIPv4(ip)) {
        networkLabel_->setText("Invalid IPv4 address");
        statusLabel_->setText(
            "Enter a valid IPv4 address to calculate."
        );

        for (int row = 0; row < results_->rowCount(); ++row) {
            results_->item(row, 1)->setText("-");
        }

        copyButton_->setEnabled(false);
        return;
    }

    // Reuse the networking core
    const SubnetInfo info = calculateSubnet(ip, prefix);

    networkLabel_->setText(
        QString::fromStdString(info.networkAddress) +
        "/" + QString::number(info.prefixLength)
    );

    statusLabel_->setText("Calculation complete.");

    const QStringList values = {
        QString::fromStdString(info.subnetMask),
        QString::fromStdString(info.networkAddress),
        QString::fromStdString(info.broadcastAddress),
        QString::fromStdString(info.firstHost),
        QString::fromStdString(info.lastHost),
        QString::number(
            static_cast<qulonglong>(info.usableHosts)
        )
    };

    for (int row = 0; row < values.size(); ++row) {
        results_->item(row, 1)->setText(values[row]);
    }

    copyButton_->setEnabled(true);
}

void SubnetCalculatorPage::copyResults() {
    QString text = "NetScope — IPv4 Subnet Calculator\n";

    text += "Network: " + networkLabel_->text() + "\n\n";

    for (int row = 0; row < results_->rowCount(); ++row) {
        text += results_->item(row, 0)->text();
        text += ": ";
        text += results_->item(row, 1)->text();
        text += "\n";
    }

    QApplication::clipboard()->setText(text);

    statusLabel_->setText(
        "Results copied to clipboard."
    );
}
