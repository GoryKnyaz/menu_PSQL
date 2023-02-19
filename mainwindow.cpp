#include "mainwindow.h"
#include "ui_mainwindow.h"

template <class T> size_t Find(std::vector<T>& mass, T value)
{
    for (size_t index(0), size(mass.size()); index != size; ++index)
        if (mass[index] == value)
            return index;
    return size_t(-1);
}

std::string getStringFromFile(std::string filename)
{
    std::ifstream file(filename);
    std::string result(""), line;
    while(!file.eof())
    {
        getline(file, line);
        result += line + '\n';
    }
    return result;
}

QList<std::string> getDataDefaultTables()
{
    std::string data(getStringFromFile("data_default_tables.txt"));
    QList<std::string> tables, result;
    size_t index, indexFind(0), size;
    while (!data.empty())
    {
        if ((indexFind = data.find(';')) == size_t(-1))
            break;
        data[0] == '\n' ?
            tables.push_back(data.substr(1, indexFind - 1))
            :
            tables.push_back(data.substr(0, indexFind));
        data = data.substr(indexFind + 1);
    }
    index = 0, size = tables.size();
    result = QList<std::string>(size, "");
    for (; index != size; ++index)
    {
        result[index] = "INSERT INTO ";
        result[index] += '"' + tables[index].substr(0, indexFind = tables[index].find('\n')) + '"';
        result[index] += " VALUES ";
        tables[index] = tables[index].substr(indexFind + 1);
        while (!tables[index].empty())
        {
            result[index] += '(' + tables[index].substr(0, indexFind = tables[index].find('\n')) + "),";
            tables[index] = indexFind == size_t(-1) ?
                    ""
                      :
                    tables[index].substr(indexFind + 1);
        }
        result[index].pop_back();
        result[index] += ';';
    }
    return result;
}

QString getInsertCommand(QString tableName, size_t size, QStringList add_data)
{
    QString result("INSERT INTO \"" + tableName + "\" VALUES (" + QString::fromStdString(std::to_string(size + 1)) + ',');
    for (size_t index(0), size(add_data.size()); index != size; ++index)
        result += add_data[index] + (index == size - 1 ? ')' : ',');
    return result + ';';
}

QString getFilterCommand(QString tableName, QStringList conditions)
{
    QString result("");
    for (size_t index(0), size(conditions.size()); index != size; ++index)
        result += "\"" + tableName + "\"." + conditions[index] + (index != size - 1 ? " AND " : "");
    return result;
}

QString getQueryFrom(const QStringList &tableNames, const QList<QStringList> relations, const QStringList &headers)
{
    QStringList cols;
    QString result(""), result_cols("SELECT DISTINCT ");
    for (size_t index(0), size(headers.size()); index != size; ++index)
        cols.append(headers[index]);
    result += " FROM \"" + tableNames[0] + '\"';
    for (size_t index(1), size(tableNames.size()); index < size; ++index)
        result += " CROSS JOIN \"" + tableNames[index] + '\"';
    for (size_t index(0), size(relations.size()); index != size; ++index)
    {
        cols[relations[index][0].toInt()] = "\"" + relations[index][2] + "\"." + relations[index][4];
        result += " JOIN \"" + relations[index][2] +  "\" ON \"" + tableNames[0] + "\"."
                + relations[index][1] + " = \"" + relations[index][2] + "\"." + relations[index][3] + ' ';
    }
    for (size_t index(0), size(cols.size()); index != size; ++index)
        result_cols += cols[index] + (index == size - 1 ? ' ' : ',' );
    return result_cols + result;
}




MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    db = QSqlDatabase::addDatabase("QPSQL");
    employeesMain = new QSqlRelationalTableModel(this, db);
    ui->empView->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    ui->empView->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);
    ui->empView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->empView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->empView->sortByColumn(0, Qt::AscendingOrder);
    QSqlRelationalDelegate* delegate = new QSqlRelationalDelegate(ui->empView);
    ui->empView->setItemDelegateForColumn(1, delegate);
    ui->empView->setItemDelegateForColumn(3, delegate);
    ui->empView->setItemDelegateForColumn(4, delegate);
    ui->empView->setItemDelegateForColumn(5, delegate);
    ui->empView->setItemDelegateForColumn(6, delegate);
    getLastConnect();
    ui->dbW->hide();
}

MainWindow::~MainWindow()
{
    setLastConnect();
    delete ui;
}

void MainWindow::getLastConnect()
{
    QStringList lastConnectList(QString::fromStdString(getStringFromFile("last_connect.txt")).split('\n'));
    lastConnectList.removeAll("");
    if (lastConnectList.size() != 3) return;
    QStringList sizeWindow(lastConnectList[2].split('x'));
    ui->login->setText(lastConnectList[0]);
    ui->password->setText(lastConnectList[1]);
    if (sizeWindow.size() == 2)
        this->resize(sizeWindow[0].toInt(), sizeWindow[1].toInt());
    else if (sizeWindow[0] == "YES")
        this->setWindowState(Qt::WindowMaximized);
}

void MainWindow::setLastConnect()
{
    std::ofstream lastConnectFile("last_connect.txt");
    lastConnectFile << ui->login->text().toStdString() << '\n';
    lastConnectFile << ui->password->text().toStdString() << '\n';
    if (this->windowState() & Qt::WindowMaximized)
        lastConnectFile << "YES";
    else
        lastConnectFile << ui->mainWidget->size().rwidth() << 'x' << ui->mainWidget->size().rheight();
    lastConnectFile.close();
}

size_t MainWindow::getSizeOfTable(QString tableName)
{
    QSqlQuery query(db);
    if (!query.exec("SELECT COUNT(*) FROM \"" + tableName + "\";"))
    {
        QMessageBox::critical(this, "Ошибка при открытии таблиц", query.lastError().text());
        return -1;
    }
    if (query.next())
        return query.value(0).toInt();
    return -1;
}

QList<QStringList> MainWindow::getDataFromTable(const QString &tableName, const QStringList &headers)
{
    QSqlQuery query(db);
    QString headers_string("SELECT ");
    for (size_t index(0), size(headers.size()); index != size; ++index)
        headers_string += headers[index] + (index == size - 1 ? ' ' : ',');
    if (!query.exec(headers_string + " FROM \"" + tableName + "\";"))
    {
        QMessageBox::critical(this, "Ошибка при открытии таблиц", query.lastError().text());
        return {};
    }
    QList<QStringList> result({});
    while (query.next())
    {
        QStringList rowData;
        for (size_t index(0), size(headers.size()); index != size; ++index)
            rowData.append(query.value(index).toString());
        result.append(rowData);
    }
    return result;
}



bool MainWindow::connection(const QString login, const QString password, const QString db_name = "postgres")
{
    db.setDatabaseName(db_name);
    db.setUserName(login);
    db.setPassword(password);
    if (!db.open())
    {
        QMessageBox::warning(this, "Ошибка при подключении к БД " + db_name, db.lastError().text());
        db.close();
        return false;
    }
    return true;
}

bool MainWindow::createDefaultTables(std::string filename)
{
    QSqlQuery query(db);
    if (!query.exec(QString::fromStdString(getStringFromFile(filename))))
    {
        QMessageBox::critical(this, "Ошибка при создании шаблонных таблиц", query.lastError().text());
        return false;
    }
    QList<std::string> commands(getDataDefaultTables());
    for (size_t index(0), size(commands.size()); index != size; ++index)
        query.exec(QString::fromStdString(commands[index]));
    return true;

}

bool MainWindow::createDefaultDatabase(const QString def_db_name)
{
    QSqlQuery query = QSqlQuery(db);
    query.exec("SELECT datname FROM pg_database WHERE datname = '" + def_db_name + "';");
    QString answer;
    if (query.next())
             answer = query.value(0).toString();
    if (answer != def_db_name)
    {
        if (!query.exec(" CREATE DATABASE \"" + def_db_name + '"' +
                        " WITH "
                        " OWNER = postgres "
                        " ENCODING = 'UTF8' "
                        " LC_COLLATE = 'Russian_Russia.1251' "
                        " LC_CTYPE = 'Russian_Russia.1251' "
                        " TABLESPACE = pg_default "
                        " CONNECTION LIMIT = -1 "
                        " IS_TEMPLATE = False;"))
        {
            QMessageBox::critical(this, "Ошибка при создании шаблонной базы данных", query.lastError().text());
            return false;
        }
    }
    return true;
}

void MainWindow::recoveryTableBegin(QString tableName, size_t id_where)
{
    QSqlQuery update(db);
    size_t size(getSizeOfTable(tableName));
    for (size_t index(id_where); index <= size; ++index)
    {
        if (!update.exec("UPDATE \"employees\" SET id = "
                         + QString::fromStdString(std::to_string(index)) + " WHERE id = "
                         + QString::fromStdString(std::to_string(index + 1))))
            break;
    }
}


void MainWindow::setupTableView(QString conditions = "")
{
    QStringList headers({"Номер", "Офис", "ФИО", "ДР", "Должность", "Оклад", "Работает с"});
    QList<QStringList> relations({{"1", "branch_id", "branches", "id", "name"}, {"4", "jobtitle_id", "jobtitles", "id", "jobtitle"}});
    employeesMain->setTable("employees");
    for (size_t index(0), size(relations.size()); index != size; ++index)
        employeesMain->setRelation(employeesMain->fieldIndex(relations[index][1]), QSqlRelation(relations[index][2], relations[index][3], relations[index][4]));
    employeesMain->setFilter(conditions);
    employeesMain->select();
    employeesMain->setEditStrategy(QSqlTableModel::OnFieldChange);
    for(int i(0); i < employeesMain->columnCount(); ++i)
        employeesMain->setHeaderData(i, Qt::Horizontal, headers[i]);
    ui->empView->setModel(employeesMain);
    ui->empView->resizeColumnsToContents();
    ui->empView->setColumnHidden(0, true);
    ui->empView->setColumnHidden(employeesMain->fieldIndex("branch_id"), false);
    ui->empView->show();
}

void MainWindow::setupTreeView()
{
    QList<QStringList> treeData(getDataFromTable("branches", {"id", "parent_id", "name"}));
    QTreeWidgetItem** items = new QTreeWidgetItem*[treeData.size()];
    QList<QTreeWidgetItem*> topItems;
    for (size_t index(0), size(treeData.size()); index != size; ++index)
        items[index] = new QTreeWidgetItem(QStringList(treeData[index][2]));
    for (size_t index(0), size(treeData.size()); index != size; ++index)
    {
        if (treeData[index][0] == treeData[index][1])
            topItems.append(items[index]);
        else
            items[treeData[index][1].toInt() - 1]->addChild(items[index]);
    }
    ui->branchTree->clear();
    ui->branchTree->addTopLevelItems(topItems);
}



void MainWindow::on_exit_clicked()
{
    MainWindow::close();
}

void MainWindow::on_enter_clicked()
{
    QString login(ui->login->text()), password(ui->password->text()), def_db_name("DefCompany");
    if (connection(login, password))
        if (createDefaultDatabase(def_db_name))
            if (connection(login, password, def_db_name))
            {
                if (createDefaultTables("default_tables.txt"))
                {
                    ui->connect->hide();
                    ui->dbW->show();
                    setupTableView();
                    setupTreeView();
                    connect(ui->empView->horizontalHeader(), &QHeaderView::sectionClicked,
                            [this](int logicalIndex)
                    {
                        size_t indexSort;
                        SortState sortLogicalIndex;
                        QString tableName("employees");
                        QStringList headerName({"id", "branch_id", "fio", "birthday", "jobtitle_id", "salary", "begin_work"});
                        if (QApplication::keyboardModifiers() & Qt::ControlModifier)
                            {
                                QString condition("1=1) ORDER BY ");
                                if ((indexSort = Find(sortColumns, std::pair<size_t, SortState>(logicalIndex,Asc))) != size_t(-1))
                                    sortColumns[indexSort].second = Desc;
                                else if ((indexSort = Find(sortColumns, std::pair<size_t, SortState>(logicalIndex, Desc))) != size_t(-1))
                                    sortColumns.erase(sortColumns.begin() + indexSort);
                                else
                                    sortColumns.push_back(std::pair<size_t, SortState>(logicalIndex, Asc));
                                if (sortColumns.size() == 0) return;
                                for (size_t index(0), size(sortColumns.size()); index != size; ++index)
                                {
                                    switch (sortColumns[index].second)
                                    {
                                    case Asc:
                                        condition.append(condition.endsWith("SC") ? ',' : ' ');
                                        condition += "\"" + tableName + "\"." + headerName[sortColumns[index].first] + " ASC";
                                        break;
                                    case Desc:
                                        condition.append(condition.endsWith("SC") ? ',' : ' ');
                                        condition += "\"" + tableName + "\"." + headerName[sortColumns[index].first] + " DESC";
                                        break;
                                    default:
                                        break;
                                    }
                                }
                                setupTableView(thisCondition + (thisCondition != "" ? " AND " : "") + condition + "--");
                                qDebug() << employeesMain->query().lastQuery();
                        }
                        else
                        {
                            sortLogicalIndex = DisableSort;
                            if ((indexSort = Find(sortColumns, std::pair<size_t, SortState>(logicalIndex,Asc))) != size_t(-1))
                                sortLogicalIndex = sortColumns[indexSort].second;
                            else if ((indexSort = Find(sortColumns, std::pair<size_t, SortState>(logicalIndex,Desc))) != size_t(-1))
                                sortLogicalIndex = sortColumns[indexSort].second;
                            sortColumns.clear();
                            switch (sortLogicalIndex)
                            {
                            case DisableSort:
                                setupTableView(thisCondition + (thisCondition != "" ? " AND " : "") + "1=1) ORDER BY \"" + tableName + "\"." + headerName[logicalIndex] + " ASC--");
                                sortColumns.push_back(std::pair<size_t, SortState>(logicalIndex, Asc));
                                break;
                            case Asc:
                                setupTableView(thisCondition + (thisCondition != "" ? " AND " : "") + "1=1) ORDER BY \"" + tableName + "\"." + headerName[logicalIndex] + " DESC--");
                                sortColumns.push_back(std::pair<size_t, SortState>(logicalIndex, Desc));
                                break;
                            case Desc:
                                setupTableView(thisCondition + (thisCondition != "" ? " AND " : "") + "1=1) ORDER BY \"" + tableName + "\".id ASC--");
                                break;
                            default:
                                break;
                            }
                            qDebug() << employeesMain->query().lastQuery();
                        }
                    });
                }
                else
                    MainWindow::close();
            }
}

void MainWindow::on_add_clicked()
{
    ed.setModal(true);
    ed.setData(db, ui->branchTree->currentItem() ? ui->branchTree->currentItem()->text(0) : "", Employee);
    ed.exec();
    if (ed.getisCorrectCard())
    {
        QSqlQuery add(db);
        if (!add.exec(getInsertCommand("employees", getSizeOfTable("employees"), ed.getEmployeeCard())))
            QMessageBox::warning(this, "Ошибка при добавлении нового сотрудника", add.lastError().text());
        employeesMain->select();
    }
}

void MainWindow::on_erase_clicked()
{
    QModelIndexList selectedList(ui->empView->selectionModel()->selectedIndexes());
    std::vector<int> selectedIndexes;
    for (size_t index(0), size(selectedList.size()); index != size; ++index)
    {
        if (Find(selectedIndexes, selectedList[index].row()) == size_t(-1))
            selectedIndexes.push_back(selectedList[index].row());
    }
    for (size_t index(0), size(selectedIndexes.size()); index != size; ++index)
    {
        int selectedRow = selectedIndexes[index];
        size_t id(employeesMain->data(employeesMain->index(selectedRow, 0)).toInt());
        if (selectedRow >= 0)
        {
            employeesMain->removeRow(selectedRow);
            recoveryTableBegin("employees", id);
            employeesMain->select();
            for (size_t index1(index + 1); index1 < size; ++index1)
                if (selectedIndexes[index1] > selectedRow)
                    selectedIndexes[index1] -= 1;
        }
    }
    setupTableView();
}

void MainWindow::on_branchTree_itemClicked(QTreeWidgetItem *item, int column)
{
    QSqlQuery query(db);
    QString col_branch;
    query.exec("SELECT id FROM \"branches\" WHERE \"branches\".name = \'" + item->text(column) + "\';");
    if (query.next())
        col_branch = query.value(0).toString();
    thisCondition = "\"employees\".branch_id = '" + col_branch + '\'';
    employeesMain->setFilter(thisCondition);
    employeesMain->select();
    ui->empView->setColumnHidden(0, true);
    ui->empView->setColumnHidden(employeesMain->fieldIndex("branch_id"), true);
    ui->empView->setModel(employeesMain);
    ui->empView->show();

}

void MainWindow::on_add_branch_clicked()
{
    ed.setModal(true);
    ed.setData(db, "", Branch);
    ed.exec();
    if (ed.getisCorrectCard())
    {
        QSqlQuery add(db);
        if (!add.exec(getInsertCommand("branches", ed.countBranch, ed.getBranchCard())))
            QMessageBox::warning(this, "Ошибка при добавлении нового офиса", add.lastError().text());
        setupTreeView();
    }
}

void MainWindow::on_exit_2_clicked()
{
    this->close();
}

void MainWindow::on_filter_clicked()
{
    ed.setModal(true);
    ed.setData(db, "", Filter);
    ed.exec();
    if (ed.getisCorrectCard())
    {
        thisCondition = getFilterCommand("employees", ed.getFilterCard());
        setupTableView(thisCondition);
    }
}

void MainWindow::on_allEpl_clicked()
{
    thisCondition = "";
    employeesMain->setFilter("");
    employeesMain->select();
    ui->empView->setModel(employeesMain);
    ui->empView->resizeColumnsToContents();
    ui->empView->setColumnHidden(employeesMain->fieldIndex("branch_id"), false);
    ui->empView->show();
}
