#include "editwindow.h"
#include "ui_editwindow.h"

QString quoteHandling(QString str)
{
    QString result(str);
    for (size_t index(0), size(result.size()); index < size; ++index)
    {
        if (result[index] == '\'')
            result.insert(++index, '\''), ++size;
        else if (str[index] == '\"')
            result.insert(++index, '\"'), ++size;
    }
    return result;
}

QStringList getDataFromTable(QSqlDatabase& db, QString data, QString table)
{
    QSqlQuery query = QSqlQuery(db);
    if (!query.exec("SELECT " + data + " FROM " + table + ";"))
    {
        QMessageBox::critical(0, "Ошибка при открытии таблиц", query.lastError().text());
        return {};
    }
    QStringList result({});
    while (query.next())
        result.append(query.value(0).toString());
    return result;
}


editwindow::editwindow(QWidget *parent) : QDialog(parent), ui(new Ui::editwindow)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::MSWindowsFixedSizeDialogHint);
}

editwindow::~editwindow()
{
    delete ui;
}


void editwindow::setCondition(QPushButton *button)
{
    uint unicode;
    QString str(button->text());
    if (str == "")
        unicode = ConditionFilter::Equal;
    else
        switch (str[0].unicode())
        {
        case Equal:
            unicode = ConditionFilter::More;
            break;
        case More:
            unicode = ConditionFilter::EqualMore;
            break;
        case EqualMore:
            unicode = ConditionFilter::Less;
            break;
        case Less:
            unicode = ConditionFilter::EqualLess;
            break;
        case EqualLess:
            unicode = ConditionFilter::DisableFilter;
            break;
        default:
            return;
        }
    button->setText(unicode == ConditionFilter::DisableFilter ? "" : QString::fromUcs4(&unicode, 1));
}

void editwindow::setData(QSqlDatabase& db, QString thisFilial, StateWidget state)
{
    QStringList branch_list(getDataFromTable(db, "name", "branches"));
    QStringList jobtitle_list(getDataFromTable(db, "jobtitle", "jobtitles"));
    branch_list.push_front("Выберете филиал");
    jobtitle_list.push_front("Выберете должность");
    isCorrectCard = false;
    ui->jobtitle_filter->clear();
    ui->branch_filter->clear();
    ui->parent_branch->clear();
    ui->jobtitle_emp->clear();
    ui->branch_emp->clear();
    ui->jobtitle_filter->addItems(jobtitle_list);
    ui->jobtitle_emp->addItems(jobtitle_list);
    ui->branch_filter->addItems(branch_list);
    ui->parent_branch->addItems(branch_list);
    ui->branch_emp->addItems(branch_list);
    ui->branch_emp->setCurrentText(thisFilial);
    countBranch = branch_list.size() - 1;
    switch (state)
    {
    case Filter:
        ui->employee_widget->hide();
        ui->branch_widget->hide();
        ui->filter_widget->show();
        break;
    case Branch:
        ui->employee_widget->hide();
        ui->branch_widget->show();
        ui->filter_widget->hide();
        break;
    case Employee:
        ui->employee_widget->show();
        ui->branch_widget->hide();
        ui->filter_widget->hide();
        break;
    default:
        break;
    }
}


bool editwindow::getisCorrectCard()
{
    return this->isCorrectCard;
}

QStringList editwindow::getEmployeeCard()
{
    return
    {
        QString::fromStdString(std::to_string(ui->branch_emp->currentIndex())),
        '\'' + quoteHandling(ui->fio_emp->text()) + '\'',
        '\'' + ui->birthday_emp->text() + '\'',
        QString::fromStdString(std::to_string(ui->jobtitle_emp->currentIndex())),
        ui->salary_emp->text(),
        '\'' + ui->begin_work_emp->text() + '\''
    };
}

QStringList editwindow::getBranchCard()
{
    return
    {
        (ui->isParent_branch->isChecked() ?
                    QString::fromStdString(std::to_string(countBranch + 1))
                    :
                    QString::fromStdString(std::to_string(ui->parent_branch->currentIndex() + 1))),
        '\'' + quoteHandling(ui->name_branch->text()) + '\''
    };
}

QStringList editwindow::getFilterCard()
{
    QStringList result; QString cond;

    if (ui->birthday_filter->isEnabled())
    {
        cond = "birthday ";
        uint sign_birthday(ui->birthday_filter_condition->text()[0].unicode());
        if (sign_birthday > 0x2000)
            cond += sign_birthday == 0x2265 ? ">=" : "<=";
        else
            cond += char(sign_birthday);
        cond += '\'' + ui->birthday_filter->text() + '\'';
        result.append(cond);
    }

    if (ui->begin_work_filter->isEnabled())
    {
        cond = "begin_work ";
        uint sign_begin_work(ui->begin_work_filter_condition->text()[0].unicode());
        if (sign_begin_work > 0x2000)
            cond += sign_begin_work == 0x2265 ? ">=" : "<=";
        else
            cond += char(sign_begin_work);
        cond += '\'' + ui->begin_work_filter->text() + '\'';
        result.append(cond);
    }

    if (ui->salary_filter->isEnabled())
    {
        cond = "salary ";
        uint sign_salary(ui->salary_filter_condition->text()[0].unicode());
        if (sign_salary > 0x2000)
            cond += sign_salary == 0x2265 ? ">=" : "<=";
        else
            cond += char(sign_salary);
        cond += ui->salary_filter->text();
        result.append(cond);
    }

    if (ui->fio_filter->text() != "")
        result.append("fio LIKE '" + quoteHandling(ui->fio_filter->text()) + '\'');

    if (ui->branch_filter->currentIndex())
        result.append("branch_id = " + QString::fromStdString(std::to_string(ui->branch_filter->currentIndex())));

    if (ui->jobtitle_filter->currentIndex())
        result.append("jobtitle_id = " + QString::fromStdString(std::to_string(ui->jobtitle_filter->currentIndex())));

    return result;
}


void editwindow::on_ok_emp_clicked()
{
    if (ui->fio_emp->text() == "")
    {
        QMessageBox::warning(this, "Недостаточно введено данных", "Для начала нужно внести ФИО.");
        return;
    }

    int age(ui->birthday_emp->date().daysTo(QDate::currentDate()) / 365.25);
    if (age < 18)
    {
        QMessageBox::warning(this, "Неправильно введены данные", "Сотрудник несовершеннолетний.");
        return;
    }
    else if (age >= 65)
    {
        QMessageBox::warning(this, "Неправильно введены данные", "У сотрудника уже пенсионный возраст.");
        return;
    }

    if (ui->branch_emp->currentIndex() == 0)
    {
        QMessageBox::warning(this, "Неправильно введены данные", "Сотрудник не прикреплен ни к одному из филиалов.");
        return;
    }

    if (ui->jobtitle_emp->currentIndex() == 0)
    {
        QMessageBox::warning(this, "Неправильно введены данные", "Сотруднику не указана должность.");
        return;
    }

    int experience(ui->begin_work_emp->date().daysTo(QDate::currentDate()) / 365.25);
    if (age - experience < 18)
    {
        QMessageBox::warning(this, "Неправильно введены данные", "Сотрудник не мог устроиться на работу несовершеннолетним");
        return;
    }

    if (ui->salary_emp->value() < 12000)
    {
        QMessageBox::warning(this, "Неправельно введены данные", "Оклад слишком маленький, он должен быть хотя бы больше прожиточного минимума)");
        return;
    }
    this->close();
    isCorrectCard = true;
}

void editwindow::on_cancel_emp_clicked()
{
    this->close();
    isCorrectCard = false;
}

void editwindow::on_ok_branch_clicked()
{
    if (ui->name_branch->text() == "")
    {
        QMessageBox::warning(this, "Недостаточно введено данных", "Для начала нужно внести название.");
        return;
    }

    if (ui->parent_branch->currentIndex() == 0 && !ui->isParent_branch->isChecked())
    {
        QMessageBox::warning(this, "Неправильно введены данные", "Не указан родитель филиала.");
        return;
    }
    this->close();
    isCorrectCard = true;
}

void editwindow::on_cancel_branch_clicked()
{
    this->close();
    isCorrectCard = false;
}

void editwindow::on_ok_filter_clicked()
{
    if (ui->fio_filter->text() != "" || ui->birthday_filter->isEnabled() || ui->branch_filter->currentIndex() ||
            ui->begin_work_filter->isEnabled() || ui->jobtitle_filter->currentIndex() || ui->salary_filter->isEnabled())
    {
        if (ui->birthday_filter->isEnabled())
        {
            int age(ui->birthday_filter->date().daysTo(QDate::currentDate()) / 365.25);
            if (age < 18)
            {
                QMessageBox::warning(this, "Неправильно введены данные", "Сотрудник несовершеннолетний.");
                return;
            }
            else if (age >= 65)
            {
                QMessageBox::warning(this, "Неправильно введены данные", "У сотрудника уже пенсионный возраст.");
                return;
            }
            if (ui->begin_work_filter->isEnabled())
            {
                int experience(ui->begin_work_filter->date().daysTo(QDate::currentDate()) / 365.25);
                if (age - experience < 18)
                {
                    QMessageBox::warning(this, "Неправильно введены данные", "Сотрудник не мог устроиться на работу несовершеннолетним");
                    return;
                }
            }
        }

        this->close();
        isCorrectCard = true;
    }
    else
        QMessageBox::warning(this, "Недостаточно введено данных", "Не указан ни одни способ фильтрации.");
}

void editwindow::on_cancel_filter_clicked()
{
    this->close();
    isCorrectCard = false;
}

void editwindow::on_isParent_branch_stateChanged(int)
{
    ui->parent_branch->setEnabled(!ui->parent_branch->isEnabled());
}

void editwindow::on_birthday_filter_condition_clicked()
{
    setCondition(ui->birthday_filter_condition);
    ui->birthday_filter->setEnabled(ui->birthday_filter_condition->text() != "");
}

void editwindow::on_salary_filter_condition_clicked()
{
    setCondition(ui->salary_filter_condition);
    ui->salary_filter->setEnabled(ui->salary_filter_condition->text() != "");
}

void editwindow::on_begin_work_filter_condition_clicked()
{
    setCondition(ui->begin_work_filter_condition);
    ui->begin_work_filter->setEnabled(ui->begin_work_filter_condition->text() != "");
}

