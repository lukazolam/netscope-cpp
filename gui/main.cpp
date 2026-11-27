
#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Create the main application window
    QMainWindow window;
    window.setWindowTitle("NetScope");
    window.resize(900, 600);

    // Main content container
    QWidget* centralWidget = new QWidget(&window);

    // Vertical layout for all elements
    QVBoxLayout* mainLayout =
        new QVBoxLayout(centralWidget);

    // Page heading
    QLabel* title = new QLabel("TCP Port Scanner");
    QFont titleFont = title->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    title->setFont(titleFont);

    mainLayout->addWidget(title);

    // Row containing the target field and button
    QHBoxLayout* inputLayout = new QHBoxLayout();

    QLineEdit* ipInput = new QLineEdit();
    ipInput->setPlaceholderText(
        "Enter target IPv4 address"
    );

    QPushButton* scanButton =
        new QPushButton("Start Scan");

    inputLayout->addWidget(ipInput);
    inputLayout->addWidget(scanButton);

    mainLayout->addLayout(inputLayout);

    // Results table
    QTableWidget* results = new QTableWidget();

    results->setColumnCount(3);
    results->setHorizontalHeaderLabels(
        {"Port", "Protocol", "Status"}
    );

    results->horizontalHeader()
        ->setSectionResizeMode(QHeaderView::Stretch);

    results->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );

    mainLayout->addWidget(results);

    // Handle button clicks
    QObject::connect(
        scanButton,
        &QPushButton::clicked,
        &window,
        [ipInput, &window]() {
            QMessageBox::information(
                &window,
                "NetScope",
                "Scan requested for: " + ipInput->text()
            );
        }
    );

    window.setCentralWidget(centralWidget);
    window.show();

    return app.exec();
}
