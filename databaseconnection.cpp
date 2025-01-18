#include "databaseconnection.h"
#include <QSqlQuery>
#include <QSqlDriver>

DatabaseConnection::DatabaseConnection() : lastExecutionTime(""), settings("DBManager", "Connections") {
}

DatabaseConnection::~DatabaseConnection() {
    disconnect();
}

bool DatabaseConnection::connect(const ConnectionParams& params) {
    if (!params.isValid()) {
        return false;
    }

    if (db.isOpen()) {
        disconnect();
    }

    QString connectionName = "DBConnection-" + QString::number(reinterpret_cast<qulonglong>(this));
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase::removeDatabase(connectionName);
    }

    currentParams = params;
    
    db = QSqlDatabase::addDatabase(params.driver, connectionName);
    
    if (params.driver == "QMYSQL") {
        QString host = params.host;
        if (host.toLower() == "localhost") {
            host = "127.0.0.1";
        }
        db.setHostName(host);
        db.setDatabaseName(params.dbName);
        db.setUserName(params.user);
        db.setPassword(params.password);
        db.setPort(params.port);
        db.setConnectOptions("MYSQL_OPT_CONNECT_TIMEOUT=20");
    } else {
        db.setHostName(params.host);
        db.setDatabaseName(params.dbName);
        db.setUserName(params.user);
        db.setPassword(params.password);
        db.setPort(params.port);
    }

    bool result = db.open();
    
    qDebug() << "Connecting to:" << db.hostName() << "port:" << db.port() 
             << "database:" << db.databaseName() << "user:" << db.userName()
             << "using driver:" << db.driverName();
             
    if (!result) {
        qDebug() << "Connection error:" << db.lastError().text();
    }
    
    return result;
}

void DatabaseConnection::disconnect() {
    QString connectionName = db.connectionName();
    if (db.isOpen()) {
        db.close();
    }
    if (!connectionName.isEmpty()) {
        QSqlDatabase::removeDatabase(connectionName);
    }
}

bool DatabaseConnection::isConnected() const {
    return db.isOpen();
}

QSqlError DatabaseConnection::lastError() const {
    return db.lastError();
}

QStringList DatabaseConnection::tables() const {
    return db.tables();
}

QSqlDatabase& DatabaseConnection::database() {
    return db;
}

bool DatabaseConnection::executeQuery(const QString& query, QSqlQuery& result) {
    if (!isConnected()) {
        return false;
    }

    QDateTime startTime = QDateTime::currentDateTime();
    
    bool isModification = query.trimmed().toUpper().startsWith("UPDATE") ||
                         query.trimmed().toUpper().startsWith("INSERT") ||
                         query.trimmed().toUpper().startsWith("DELETE");
    
    if (isModification && !db.transaction()) {
        qDebug() << "Failed to start transaction:" << db.lastError().text();
        return false;
    }
    
    result = QSqlQuery(db);
    bool success = result.exec(query);
    
    if (isModification) {
        if (success) {
            if (!db.commit()) {
                qDebug() << "Failed to commit transaction:" << db.lastError().text();
                db.rollback();
                success = false;
            }
        } else {
            db.rollback();
        }
    }
    
    QDateTime endTime = QDateTime::currentDateTime();
    lastExecutionTime = QString("%1 ms").arg(startTime.msecsTo(endTime));
    
    if (!success) {
        qDebug() << "Query error:" << result.lastError().text();
    }
    
    return success;
}

QString DatabaseConnection::getLastExecutionTime() const {
    return lastExecutionTime;
}

QStringList DatabaseConnection::getDatabases() const {
    if (!isConnected()) {
        return QStringList();
    }

    QStringList databases;
    QSqlQuery query(db);
    
    if (db.driverName() == "QMYSQL") {
        if (query.exec("SHOW DATABASES")) {
            while (query.next()) {
                databases << query.value(0).toString();
            }
        }
    } else if (db.driverName() == "QPSQL") {
        if (query.exec("SELECT datname FROM pg_database WHERE datistemplate = false")) {
            while (query.next()) {
                databases << query.value(0).toString();
            }
        }
    }
    
    return databases;
}

bool DatabaseConnection::changeDatabase(const QString& dbName) {
    if (!isConnected()) {
        return false;
    }
    
    ConnectionParams params = currentParams;
    params.dbName = dbName;
    
    return connect(params);
}

QStringList DatabaseConnection::getAvailableDrivers() {
    return QSqlDatabase::drivers();
}

void DatabaseConnection::saveConnectionSettings(const QString& serverName, const ConnectionParams& params) {
    settings.beginGroup(serverName);
    settings.setValue("driver", params.driver);
    settings.setValue("host", params.host);
    settings.setValue("port", params.port);
    settings.setValue("dbName", params.dbName);
    settings.setValue("user", params.user);
    settings.setValue("password", params.password);
    settings.endGroup();
}

DatabaseConnection::ConnectionParams DatabaseConnection::loadConnectionSettings(const QString& serverName) {
    ConnectionParams params;
    settings.beginGroup(serverName);
    params.driver = settings.value("driver").toString();
    params.host = settings.value("host").toString();
    params.port = settings.value("port").toInt();
    params.dbName = settings.value("dbName").toString();
    params.user = settings.value("user").toString();
    params.password = settings.value("password").toString();
    settings.endGroup();
    return params;
}

QStringList DatabaseConnection::getSavedServers() const {
    return settings.childGroups();
}

void DatabaseConnection::removeServerSettings(const QString& serverName) {
    settings.remove(serverName);
}
