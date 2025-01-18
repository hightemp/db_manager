#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "serversdialog.h"
#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QClipboard>
#include <QDateTime>
#include <QSqlRecord>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , settings("DBManager", "MainWindow")
{
    ui->setupUi(this);
    setupUI();
    setupMenus();
    setupToolbar();
    setupStatusBar();
    loadServers();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupUI()
{
    mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(mainSplitter);

    serversTree = new QTreeWidget(mainSplitter);
    serversTree->setHeaderLabels({tr("Серверы")});
    serversTree->setContextMenuPolicy(Qt::CustomContextMenu);

    auto rightPanel = new QWidget(mainSplitter);
    auto rightLayout = new QVBoxLayout(rightPanel);

    queryEdit = new QTextEdit(rightPanel);
    queryEdit->setPlaceholderText(tr("Введите SQL запрос..."));
    rightLayout->addWidget(queryEdit);

    auto executeButton = new QPushButton(tr("Выполнить"), rightPanel);
    rightLayout->addWidget(executeButton);

    dataTable = new QTableWidget(rightPanel);
    dataTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    dataTable->setSortingEnabled(false);
    dataTable->horizontalHeader()->setSectionsClickable(true);
    rightLayout->addWidget(dataTable);

    dataTable->setContextMenuPolicy(Qt::CustomContextMenu);
    tableContextMenu = new QMenu(this);
    tableContextMenu->addAction(tr("Копировать"), this, &MainWindow::copySelectedCells);
    tableContextMenu->addAction(tr("Экспорт"), this, &MainWindow::exportToFile);

    connect(serversTree, &QTreeWidget::itemDoubleClicked, this, &MainWindow::handleTreeItemDoubleClick);
    connect(serversTree, &QTreeWidget::customContextMenuRequested, this, &MainWindow::showContextMenu);
    connect(dataTable, &QTableWidget::customContextMenuRequested,
            [this](const QPoint &pos) { tableContextMenu->exec(dataTable->mapToGlobal(pos)); });
    connect(executeButton, &QPushButton::clicked, this, &MainWindow::executeQuery);
    connect(dataTable, &QTableWidget::cellChanged, this, &MainWindow::onCellChanged);
    connect(dataTable->horizontalHeader(), SIGNAL(sectionClicked(int)),
            this, SLOT(onHeaderClicked(int)));

    QByteArray splitterState = settings.value("splitterState").toByteArray();
    if (!splitterState.isEmpty()) {
        mainSplitter->restoreState(splitterState);
    } else {
        mainSplitter->setSizes({200, 600});
    }
}

void MainWindow::setupMenus()
{
    auto fileMenu = menuBar()->addMenu(tr("Файл"));
    fileMenu->addAction(tr("Добавить сервер"), this, &MainWindow::addServer);
    fileMenu->addAction(tr("Настройки"), this, &MainWindow::showSettings);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Выход"), this, &QWidget::close);
}

void MainWindow::setupToolbar()
{
    auto toolbar = addToolBar(tr("Основная панель"));
    toolbar->addAction(tr("Добавить сервер"), this, &MainWindow::addServer);
    toolbar->addAction(tr("Настройки"), this, &MainWindow::showSettings);
}

void MainWindow::setupStatusBar()
{
    statusLabel = new QLabel(this);
    executionTimeLabel = new QLabel(this);
    statusBar()->addWidget(statusLabel);
    statusBar()->addPermanentWidget(executionTimeLabel);
}

void MainWindow::loadServers()
{
    serversTree->clear();
    QStringList servers = dbConnection.getSavedServers();
    for (const QString &serverName : servers) {
        auto serverItem = new QTreeWidgetItem(serversTree);
        serverItem->setText(0, serverName);
        serverItem->setIcon(0, QIcon::fromTheme("network-server"));
    }
}

void MainWindow::saveServers()
{
    settings.setValue("splitterState", mainSplitter->saveState());
}

void MainWindow::addServer()
{
    ServersDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString serverName = dialog.getServerName();
        DatabaseConnection::ConnectionParams params = dialog.getConnectionParams();
        dbConnection.saveConnectionSettings(serverName, params);
        
        auto serverItem = new QTreeWidgetItem(serversTree);
        serverItem->setText(0, serverName);
        serverItem->setIcon(0, QIcon::fromTheme("network-server"));
    }
}

void MainWindow::editServer()
{
    auto item = serversTree->currentItem();
    if (!item) return;

    QString serverName = item->text(0);
    ServersDialog dialog(this, serverName);
    if (dialog.exec() == QDialog::Accepted) {
        QString newServerName = dialog.getServerName();
        DatabaseConnection::ConnectionParams params = dialog.getConnectionParams();
        
        if (serverName != newServerName) {
            dbConnection.removeServerSettings(serverName);
        }
        dbConnection.saveConnectionSettings(newServerName, params);
        
        item->setText(0, newServerName);
    }
}

void MainWindow::removeServer()
{
    auto item = serversTree->currentItem();
    if (!item) return;

    QString serverName = item->text(0);
    if (QMessageBox::question(this, tr("Удаление сервера"),
                            tr("Вы уверены, что хотите удалить сервер %1?").arg(serverName)) 
        == QMessageBox::Yes) {
        dbConnection.removeServerSettings(serverName);
        delete item;
    }
}

void MainWindow::connectToServer(QTreeWidgetItem *item)
{
    if (!item) return;

    QString serverName = item->text(0);
    DatabaseConnection::ConnectionParams params = dbConnection.loadConnectionSettings(serverName);
    
    if (dbConnection.isConnected()) {
        dbConnection.disconnect();
    }
    
    if (dbConnection.connect(params)) {
        updateServerStatus(item, true);
        
        item->takeChildren();
        dataTable->clear();
        dataTable->setRowCount(0);
        dataTable->setColumnCount(0);
        
        QStringList databases = dbConnection.getDatabases();
        for (const QString &dbName : databases) {
            auto dbItem = new QTreeWidgetItem(item);
            dbItem->setText(0, dbName);
            dbItem->setIcon(0, QIcon::fromTheme("folder-database"));
        }
        
        item->setExpanded(true);
    } else {
        updateServerStatus(item, false);
        QMessageBox::critical(this, tr("Ошибка"),
                            tr("Не удалось подключиться к серверу: %1")
                            .arg(dbConnection.lastError().text()));
    }
}

void MainWindow::updateServerStatus(QTreeWidgetItem *serverItem, bool connected)
{
    if (connected) {
        serverItem->setIcon(0, QIcon::fromTheme("network-server"));
        statusLabel->setText(tr("Подключено к %1").arg(serverItem->text(0)));
    } else {
        serverItem->setIcon(0, QIcon::fromTheme("network-offline"));
        statusLabel->setText(tr("Отключено"));
    }
}

void MainWindow::executeQuery()
{
    QString query = queryEdit->toPlainText().trimmed();
    if (query.isEmpty()) {
        QMessageBox::warning(this, tr("Предупреждение"), tr("Введите SQL запрос"));
        return;
    }

    QSqlQuery result;
    if (dbConnection.executeQuery(query, result)) {
        dataTable->clear();
        dataTable->setRowCount(0);
        
        QStringList headers;
        QSqlRecord record = result.record();
        int columnCount = record.count();
        for (int i = 0; i < columnCount; ++i) {
            headers << record.fieldName(i);
        }
        dataTable->setColumnCount(columnCount);
        dataTable->setHorizontalHeaderLabels(headers);

        while (result.next()) {
            int row = dataTable->rowCount();
            dataTable->insertRow(row);
            for (int col = 0; col < columnCount; ++col) {
                auto item = new QTableWidgetItem(result.value(col).toString());
                dataTable->setItem(row, col, item);
            }
        }

        statusLabel->setText(tr("Запрос выполнен успешно"));
        executionTimeLabel->setText(dbConnection.getLastExecutionTime());
    } else {
        QMessageBox::critical(this, tr("Ошибка"),
                            tr("Ошибка выполнения запроса: %1")
                            .arg(dbConnection.lastError().text()));
    }
}

void MainWindow::loadDatabaseTables(QTreeWidgetItem *dbItem)
{
    if (!dbItem || !dbItem->parent()) return;
    
    QString dbName = dbItem->text(0);
    if (dbConnection.changeDatabase(dbName)) {
        dbItem->takeChildren();
        
        QStringList tables = dbConnection.tables();
        for (const QString &table : tables) {
            auto tableItem = new QTreeWidgetItem(dbItem);
            tableItem->setText(0, table);
            tableItem->setIcon(0, QIcon::fromTheme("text-x-generic"));
        }
        
        dbItem->setExpanded(true);
        statusLabel->setText(tr("База данных %1 загружена").arg(dbName));
    } else {
        QMessageBox::critical(this, tr("Ошибка"),
                            tr("Не удалось подключиться к базе данных: %1")
                            .arg(dbConnection.lastError().text()));
    }
}

void MainWindow::showTableData(QTreeWidgetItem *item)
{
    if (!item || !item->parent() || !item->parent()->parent()) return;
    
    QString tableName = item->text(0);
    QString query;
    
    if (dbConnection.database().driverName() == "QPSQL") {
        query = QString("SELECT * FROM \"%1\"").arg(tableName);
    } else {
        query = QString("SELECT * FROM `%1`").arg(tableName);
    }
    
    QSqlQuery result;
    if (dbConnection.executeQuery(query, result)) {
        dataTable->setUpdatesEnabled(false);
        
        dataTable->clear();
        dataTable->setRowCount(0);
        
        QStringList headers;
        QSqlRecord record = result.record();
        int columnCount = record.count();
        for (int i = 0; i < columnCount; ++i) {
            headers << record.fieldName(i);
        }
        dataTable->setColumnCount(columnCount);
        dataTable->setHorizontalHeaderLabels(headers);

        result.last();
        int totalRows = result.at() + 1;
        result.first();
        result.previous();
        
        dataTable->setRowCount(totalRows);

        int row = 0;
        while (result.next()) {
            for (int col = 0; col < columnCount; ++col) {
                auto item = new QTableWidgetItem(result.value(col).toString());
                dataTable->setItem(row, col, item);
            }
            row++;
        }

        dataTable->setUpdatesEnabled(true);
        dataTable->viewport()->update();
        statusLabel->setText(tr("Данные загружены"));
        executionTimeLabel->setText(dbConnection.getLastExecutionTime());
    } else {
        QMessageBox::critical(this, tr("Ошибка"),
                            tr("Ошибка загрузки данных: %1")
                            .arg(dbConnection.lastError().text()));
    }
}

void MainWindow::showContextMenu(const QPoint &pos)
{
    auto item = serversTree->itemAt(pos);
    if (!item) return;

    QMenu menu(this);
    if (!item->parent()) {
        menu.addAction(tr("Подключиться"), [this, item]() { connectToServer(item); });
        menu.addAction(tr("Редактировать"), this, &MainWindow::editServer);
        menu.addAction(tr("Удалить"), this, &MainWindow::removeServer);
    }
    menu.exec(serversTree->mapToGlobal(pos));
}

void MainWindow::showSettings()
{
    SettingsDialog dialog(this);
    dialog.exec();
}

void MainWindow::copySelectedCells()
{
    if (dataTable->selectedRanges().isEmpty()) return;
    
    QString text;
    QTableWidgetSelectionRange range = dataTable->selectedRanges().at(0);
    
    for (int i = range.topRow(); i <= range.bottomRow(); ++i) {
        for (int j = range.leftColumn(); j <= range.rightColumn(); ++j) {
            if (j > range.leftColumn()) text += "\t";
            text += dataTable->item(i, j)->text();
        }
        text += "\n";
    }
    
    QApplication::clipboard()->setText(text);
}

void MainWindow::exportToFile()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Экспорт данных"),
                                                  QString(), tr("CSV файлы (*.csv)"));
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось открыть файл для записи"));
        return;
    }

    QTextStream stream(&file);
    
    for (int i = 0; i < dataTable->columnCount(); ++i) {
        if (i > 0) stream << ",";
        stream << "\"" << dataTable->horizontalHeaderItem(i)->text() << "\"";
    }
    stream << "\n";

    for (int row = 0; row < dataTable->rowCount(); ++row) {
        for (int col = 0; col < dataTable->columnCount(); ++col) {
            if (col > 0) stream << ",";
            QString value = dataTable->item(row, col)->text();
            value.replace("\"", "\"\"");
            stream << "\"" << value << "\"";
        }
        stream << "\n";
    }

    file.close();
    statusLabel->setText(tr("Данные экспортированы в %1").arg(fileName));
}

QString MainWindow::getCurrentTableName() const
{
    auto item = serversTree->currentItem();
    if (!item || !item->parent()) return QString();
    return item->text(0);
}

void MainWindow::onCellChanged(int row, int column)
{
    dataTable->blockSignals(true);
    
    QString tableName = getCurrentTableName();
    if (tableName.isEmpty()) {
        dataTable->blockSignals(false);
        return;
    }
    
    QTableWidgetItem *item = dataTable->item(row, column);
    if (!item) {
        dataTable->blockSignals(false);
        return;
    }
    
    QString oldValue = item->data(Qt::UserRole).toString();
    if (oldValue.isEmpty()) {
        oldValue = item->text();
    }
    
    QStringList columnNames;
    for (int i = 0; i < dataTable->columnCount(); ++i) {
        QString headerText = dataTable->horizontalHeaderItem(i)->text();
        headerText.remove(" ▲").remove(" ▼");
        columnNames << headerText;
    }
    
    QString whereClause;
    QString quotedFormat = dbConnection.database().driverName() == "QPSQL" ? "\"%1\" = '%2'" : "`%1` = '%2'";
    
    QMap<QString, QString> rowValues;
    for (int i = 0; i < dataTable->columnCount(); ++i) {
        QTableWidgetItem *rowItem = dataTable->item(row, i);
        if (!rowItem) continue;
        
        QString value = (i == column) ? oldValue : rowItem->text();
        rowValues[columnNames[i]] = value;
        
        if (!whereClause.isEmpty()) whereClause += " AND ";
        whereClause += quotedFormat
            .arg(columnNames[i])
            .arg(value.replace("'", "''"));
    }
    
    QString updateQuery;
    if (dbConnection.database().driverName() == "QPSQL") {
        updateQuery = QString("UPDATE \"%1\" SET \"%2\" = '%3' WHERE %4")
            .arg(tableName)
            .arg(columnNames[column])
            .arg(item->text().replace("'", "''"))
            .arg(whereClause);
    } else {
        updateQuery = QString("UPDATE `%1` SET `%2` = '%3' WHERE %4")
            .arg(tableName)
            .arg(columnNames[column])
            .arg(item->text().replace("'", "''"))
            .arg(whereClause);
    }
    
    qDebug() << "Executing query:" << updateQuery;
    
    QSqlQuery result;
    if (!dbConnection.executeQuery(updateQuery, result)) {
        QMessageBox::critical(this, tr("Ошибка"),
                            tr("Не удалось обновить данные: %1")
                            .arg(dbConnection.lastError().text()));
        item->setText(oldValue);
    } else {
        item->setData(Qt::UserRole, item->text());
        statusLabel->setText(tr("Данные успешно обновлены"));
    }
    
    dataTable->blockSignals(false);
}

void MainWindow::onHeaderClicked(int logicalIndex)
{
    static Qt::SortOrder currentOrder = Qt::AscendingOrder;
    static int lastColumn = -1;
    
    if (lastColumn == logicalIndex) {
        currentOrder = (currentOrder == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
    } else {
        currentOrder = Qt::AscendingOrder;
        lastColumn = logicalIndex;
    }
    
    sortTable(logicalIndex, currentOrder);
}

void MainWindow::sortTable(int column, Qt::SortOrder order)
{
    dataTable->setUpdatesEnabled(false);
    
    int rowCount = dataTable->rowCount();
    int colCount = dataTable->columnCount();
    
    QVector<QPair<QStringList, int>> data;
    data.reserve(rowCount);
    
    for (int row = 0; row < rowCount; ++row) {
        QStringList rowData;
        rowData.reserve(colCount);
        
        for (int col = 0; col < colCount; ++col) {
            QTableWidgetItem *item = dataTable->item(row, col);
            rowData << (item ? item->text() : QString());
        }
        data.append({rowData, row});
    }
    
    std::sort(data.begin(), data.end(), 
        [column, order](const QPair<QStringList, int> &a, const QPair<QStringList, int> &b) {
            QString val1 = a.first.value(column);
            QString val2 = b.first.value(column);
            
            bool ok1, ok2;
            double num1 = val1.toDouble(&ok1);
            double num2 = val2.toDouble(&ok2);
            
            if (ok1 && ok2) {
                return order == Qt::AscendingOrder ? num1 < num2 : num1 > num2;
            } else {
                return order == Qt::AscendingOrder ? 
                    val1.localeAwareCompare(val2) < 0 : 
                    val1.localeAwareCompare(val2) > 0;
            }
        }
    );
    
    QVector<QVector<QTableWidgetItem*>> sortedData(rowCount, QVector<QTableWidgetItem*>(colCount));
    
    for (int row = 0; row < rowCount; ++row) {
        for (int col = 0; col < colCount; ++col) {
            sortedData[row][col] = dataTable->takeItem(row, col);
        }
    }
    
    for (int newRow = 0; newRow < data.size(); ++newRow) {
        int oldRow = data[newRow].second;
        for (int col = 0; col < colCount; ++col) {
            dataTable->setItem(newRow, col, sortedData[oldRow][col]);
        }
    }
    
    QHeaderView* header = dataTable->horizontalHeader();
    header->setSortIndicator(column, order);
    header->setSortIndicatorShown(true);
    
    dataTable->setUpdatesEnabled(true);
    dataTable->viewport()->update();
    
    statusLabel->setText(tr("Данные отсортированы"));
}

void MainWindow::handleTreeItemDoubleClick(QTreeWidgetItem *item, int /*column*/)
{
    if (!item) return;

    if (!item->parent()) {
        connectToServer(item);
    } else if (!item->parent()->parent()) {
        loadDatabaseTables(item);
    } else {
        showTableData(item);
    }
}
