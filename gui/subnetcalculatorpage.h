
#pragma once

#include <QWidget>

class QLineEdit;
class QSpinBox;
class QLabel;
class QTableWidget;
class QPushButton;

class SubnetCalculatorPage : public QWidget {
public:
    explicit SubnetCalculatorPage(
        QWidget* parent = nullptr
    );

private:
    void calculate();
    void copyResults();

    QLineEdit* ipInput_ = nullptr;
    QSpinBox* prefixInput_ = nullptr;

    QLabel* statusLabel_ = nullptr;
    QLabel* networkLabel_ = nullptr;

    QTableWidget* results_ = nullptr;
    QPushButton* copyButton_ = nullptr;
};
