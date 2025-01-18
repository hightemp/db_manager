#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QSplitter>
#include <QLabel>
#include <QSettings>
#include <QMenu>
#include <QPushButton>
#include <QHeaderView>
#include "databaseconnection.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void addServer();
    void editServer();
    void removeServer();
    void connectToServer(QTreeWidgetItem *item);
    void executeQuery();
    void showSettings();
    void handleTreeItemDoubleClick(QTreeWidgetItem *item, int column);
    void showContextMenu(const QPoint &pos);
    void copySelectedCells();
    void exportToFile();
    void onCellChanged(int row, int column);
    void onHeaderClicked(int logicalIndex);

private:
    void setupUI();
    void setupMenus();
    void setupToolbar();
    void setupStatusBar();
    void loadServers();
    void saveServers();
    void updateServerStatus(QTreeWidgetItem *serverItem, bool connected);
    QString getDefaultDriver();
    QString getCurrentTableName() const;
    void sortTable(int column, Qt::SortOrder order);
    void loadDatabaseTables(QTreeWidgetItem *dbItem);
    void showTableData(QTreeWidgetItem *item);

    Ui::MainWindow *ui;
    QSplitter *mainSplitter;
    QTreeWidget *serversTree;
    QTableWidget *dataTable;
    QTextEdit *queryEdit;
    QLabel *statusLabel;
    QLabel *executionTimeLabel;
    DatabaseConnection dbConnection;
    QSettings settings;
    QMenu *tableContextMenu;
};
#endif // MAINWINDOW_H
