#include "settingsdialog.h"
#include "databaseconnection.h"

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent), settings("DBManager", "Settings")
{
    setWindowTitle(tr("Настройки"));
    setMinimumWidth(300);

    auto layout = new QVBoxLayout(this);
    
    auto label = new QLabel(tr("Провайдер БД по умолчанию:"), this);
    layout->addWidget(label);

    driverComboBox = new QComboBox(this);
    QStringList drivers = DatabaseConnection::getAvailableDrivers();
    for (const QString& driver : drivers) {
        if (driver.contains("MYSQL", Qt::CaseInsensitive) || 
            driver.contains("PSQL", Qt::CaseInsensitive)) {
            driverComboBox->addItem(driver);
        }
    }
    layout->addWidget(driverComboBox);

    auto buttonLayout = new QHBoxLayout;
    saveButton = new QPushButton(tr("Сохранить"), this);
    cancelButton = new QPushButton(tr("Отмена"), this);
    
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    connect(saveButton, &QPushButton::clicked, this, &SettingsDialog::saveSettings);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    loadSettings();
}

SettingsDialog::~SettingsDialog() {}

QString SettingsDialog::getDefaultDriver() const {
    return settings.value("defaultDriver", "QMYSQL").toString();
}

void SettingsDialog::saveSettings() {
    settings.setValue("defaultDriver", driverComboBox->currentText());
    accept();
}

void SettingsDialog::loadSettings() {
    QString defaultDriver = getDefaultDriver();
    int index = driverComboBox->findText(defaultDriver);
    if (index != -1) {
        driverComboBox->setCurrentIndex(index);
    }
}
