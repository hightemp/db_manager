#ifndef SERVERSDIALOG_H
#define SERVERSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include "databaseconnection.h"

class ServersDialog : public QDialog {
    Q_OBJECT

public:
    explicit ServersDialog(QWidget *parent = nullptr, const QString &serverName = QString());
    ~ServersDialog();

    DatabaseConnection::ConnectionParams getConnectionParams() const;
    QString getServerName() const;

private slots:
    void testConnection();
    void validateAndAccept();
    void updateUI();

private:
    void setupUI();
    bool validateInputs();

    QLineEdit *serverNameEdit;
    QComboBox *driverComboBox;
    QLineEdit *hostEdit;
    QSpinBox *portSpinBox;
    QLineEdit *dbNameEdit;
    QLineEdit *userEdit;
    QLineEdit *passwordEdit;
    QPushButton *testButton;
    QPushButton *saveButton;
    QPushButton *cancelButton;
    QLabel *statusLabel;

    QString editingServerName;
    DatabaseConnection dbConnection;
};

#endif // SERVERSDIALOG_H
