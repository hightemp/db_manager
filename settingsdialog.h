#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>
#include <QLabel>

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    QString getDefaultDriver() const;

private slots:
    void saveSettings();
    void loadSettings();

private:
    QComboBox *driverComboBox;
    QPushButton *saveButton;
    QPushButton *cancelButton;
    QSettings settings;
};

#endif // SETTINGSDIALOG_H
