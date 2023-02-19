#ifndef EDITWINDOW_H
#define EDITWINDOW_H
#pragma once

#include <string>

#include <QChar>
#include <QDialog>
#include <QDateTime>
#include <QMessageBox>

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlDatabase>

namespace Ui { class editwindow; }

enum StateWidget
{
    Filter,
    Branch,
    Employee
};

enum ConditionFilter
{
    DisableFilter = uint('\0'),
    Equal = uint('='),
    More = uint('>'),
    EqualMore = uint(0x2265),
    Less = uint('<'),
    EqualLess = uint(0x2264)
};

class editwindow : public QDialog
{
    Q_OBJECT

public:
    explicit editwindow(QWidget *parent = nullptr);
    ~editwindow();

    void setCondition(QPushButton *);
    void setData(QSqlDatabase&, QString, StateWidget);

    bool getisCorrectCard();
    QStringList getEmployeeCard();
    QStringList getBranchCard();
    QStringList getFilterCard();

    size_t countBranch = 0;
private slots:
    void on_ok_emp_clicked();
    void on_ok_branch_clicked();
    void on_cancel_emp_clicked();
    void on_cancel_branch_clicked();
    void on_isParent_branch_stateChanged(int arg1);
    void on_ok_filter_clicked();
    void on_cancel_filter_clicked();
    void on_birthday_filter_condition_clicked();
    void on_salary_filter_condition_clicked();
    void on_begin_work_filter_condition_clicked();

private:
    Ui::editwindow *ui;
    bool isCorrectCard = false;
};

#endif // EDITWINDOW_H
