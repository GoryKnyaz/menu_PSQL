#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#pragma once

#include <vector>
#include <fstream>

#include <QSqlRelation>
#include <QSqlTableModel>
#include <QSqlQueryModel>
#include <QSqlRelationalDelegate>

#include <QPainter>
#include <QModelIndex>
#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QTableWidgetItem>
#include <QAbstractItemView>

#include "editwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

enum SortState
{
    DisableSort,
    Asc,
    Desc,
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void getLastConnect();
    void setLastConnect();
    size_t getSizeOfTable(QString);
    QList<QStringList> getDataFromTable(const QString &, const QStringList &);

    bool connection(const QString, const QString, const QString);
    bool createDefaultTables(std::string);
    bool createDefaultDatabase(const QString);
    void recoveryTableBegin(QString tableName, size_t id_where);

    void setupTableView(QString);
    void setupTreeView();

    void on_exit_clicked();
    void on_enter_clicked();
    void on_add_clicked();
    void on_erase_clicked();
    void on_branchTree_itemClicked(QTreeWidgetItem *, int);
    void on_add_branch_clicked();
    void on_exit_2_clicked();
    void on_filter_clicked();
    void on_allEpl_clicked();

private:
    editwindow ed;
    QSqlDatabase db;
    Ui::MainWindow *ui;
    QString thisCondition;
    QSqlRelationalTableModel* employeesMain;
    std::vector<std::pair<size_t, SortState>> sortColumns;
};
#endif // MAINWINDOW_H
