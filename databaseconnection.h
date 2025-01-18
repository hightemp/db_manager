#ifndef DATABASECONNECTION_H
#define DATABASECONNECTION_H

#include <QString>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include <QDateTime>
#include <QSettings>

class DatabaseConnection {
public:
    struct ConnectionParams {
        QString driver;
        QString host;
        QString dbName;
        QString user;
        QString password;
        int port;
        
        bool isValid() const {
            return !driver.isEmpty() && !host.isEmpty() && 
                   !dbName.isEmpty() && !user.isEmpty() && 
                   port > 0 && port < 65536;
        }
    };

    DatabaseConnection();
    ~DatabaseConnection();

    bool connect(const ConnectionParams& params);
    void disconnect();
    bool isConnected() const;
    QSqlError lastError() const;
    QStringList tables() const;
    QSqlDatabase& database();
    
    bool executeQuery(const QString& query, QSqlQuery& result);
    QString getLastExecutionTime() const;
    QStringList getDatabases() const;
    bool changeDatabase(const QString& dbName);
    
    static QStringList getAvailableDrivers();
    
    void saveConnectionSettings(const QString& serverName, const ConnectionParams& params);
    ConnectionParams loadConnectionSettings(const QString& serverName);
    QStringList getSavedServers() const;
    void removeServerSettings(const QString& serverName);
    
private:
    QString lastExecutionTime;
    QSettings settings;

private:
    QSqlDatabase db;
    ConnectionParams currentParams;
};

#endif // DATABASECONNECTION_H
