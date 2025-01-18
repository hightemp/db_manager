#include "serversdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QSettings>

ServersDialog::ServersDialog(QWidget *parent, const QString &serverName)
    : QDialog(parent), editingServerName(serverName)
{
    setupUI();
    if (!serverName.isEmpty()) {
        DatabaseConnection::ConnectionParams params = dbConnection.loadConnectionSettings(serverName);
        serverNameEdit->setText(serverName);
        int driverIndex = driverComboBox->findText(params.driver);
        if (driverIndex != -1) {
            driverComboBox->setCurrentIndex(driverIndex);
        }
        hostEdit->setText(params.host);
        portSpinBox->setValue(params.port);
        dbNameEdit->setText(params.dbName);
        userEdit->setText(params.user);
        passwordEdit->setText(params.password);
        
        setWindowTitle(tr("Edit Server"));
    } else {
        setWindowTitle(tr("Add Server"));
    }
}

ServersDialog::~ServersDialog() {}

void ServersDialog::setupUI() {
    auto mainLayout = new QVBoxLayout(this);
    auto formLayout = new QFormLayout;

    serverNameEdit = new QLineEdit(this);
    driverComboBox = new QComboBox(this);
    QStringList drivers = DatabaseConnection::getAvailableDrivers();
    for (const QString& driver : drivers) {
        if (driver.contains("MYSQL", Qt::CaseInsensitive) || 
            driver.contains("PSQL", Qt::CaseInsensitive)) {
            driverComboBox->addItem(driver);
        }
    }
    
    hostEdit = new QLineEdit(this);
    portSpinBox = new QSpinBox(this);
    portSpinBox->setRange(1, 65535);
    portSpinBox->setValue(3306);
    
    dbNameEdit = new QLineEdit(this);
    userEdit = new QLineEdit(this);
    passwordEdit = new QLineEdit(this);
    passwordEdit->setEchoMode(QLineEdit::Password);
    
    formLayout->addRow(tr("Server Name:"), serverNameEdit);
    formLayout->addRow(tr("Database Type:"), driverComboBox);
    formLayout->addRow(tr("Host:"), hostEdit);
    formLayout->addRow(tr("Port:"), portSpinBox);
    formLayout->addRow(tr("Database:"), dbNameEdit);
    formLayout->addRow(tr("Username:"), userEdit);
    formLayout->addRow(tr("Password:"), passwordEdit);
    
    mainLayout->addLayout(formLayout);
    
    statusLabel = new QLabel(this);
    mainLayout->addWidget(statusLabel);
    
    auto buttonLayout = new QHBoxLayout;
    testButton = new QPushButton(tr("Test Connection"), this);
    saveButton = new QPushButton(tr("Save"), this);
    cancelButton = new QPushButton(tr("Cancel"), this);
    
    buttonLayout->addWidget(testButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    connect(testButton, &QPushButton::clicked, this, &ServersDialog::testConnection);
    connect(saveButton, &QPushButton::clicked, this, &ServersDialog::validateAndAccept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(driverComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ServersDialog::updateUI);
            
    setMinimumWidth(400);
}

void ServersDialog::updateUI() {
    if (driverComboBox->currentText().contains("MYSQL", Qt::CaseInsensitive)) {
        if (portSpinBox->value() == 5432) {
            portSpinBox->setValue(3306);
        }
    } else if (driverComboBox->currentText().contains("PSQL", Qt::CaseInsensitive)) {
        if (portSpinBox->value() == 3306) {
            portSpinBox->setValue(5432);
        }
    }
}

void ServersDialog::testConnection() {
    if (!validateInputs()) {
        return;
    }
    
    DatabaseConnection::ConnectionParams params = getConnectionParams();
    if (dbConnection.connect(params)) {
        statusLabel->setText(tr("Connection successful!"));
        statusLabel->setStyleSheet("color: green;");
        dbConnection.disconnect();
    } else {
        statusLabel->setText(tr("Connection error: %1").arg(dbConnection.lastError().text()));
        statusLabel->setStyleSheet("color: red;");
    }
}

void ServersDialog::validateAndAccept() {
    if (!validateInputs()) {
        return;
    }
    accept();
}

bool ServersDialog::validateInputs() {
    if (serverNameEdit->text().isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Enter server name"));
        return false;
    }
    if (hostEdit->text().isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Enter host address"));
        return false;
    }
    if (dbNameEdit->text().isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Enter database name"));
        return false;
    }
    if (userEdit->text().isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Enter username"));
        return false;
    }
    return true;
}

DatabaseConnection::ConnectionParams ServersDialog::getConnectionParams() const {
    DatabaseConnection::ConnectionParams params;
    params.driver = driverComboBox->currentText();
    params.host = hostEdit->text();
    params.port = portSpinBox->value();
    params.dbName = dbNameEdit->text();
    params.user = userEdit->text();
    params.password = passwordEdit->text();
    return params;
}

QString ServersDialog::getServerName() const {
    return serverNameEdit->text();
}
