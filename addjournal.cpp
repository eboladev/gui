#include "addjournal.h"
#include "ui_addjournal.h"
#include "QDebug"
#include "QSqlQuery"
#include "QMessageBox"
AddJournal::AddJournal(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddJournal)
{
    ui->setupUi(this);
    this->setWindowTitle("Add Journal");
}

AddJournal::~AddJournal()
{
    delete ui;
}

void AddJournal::on_buttonBox_accepted()
{

   QString jName = ui->lineEdit->text();

   if( jName.trimmed() != ""){
        QString querystring = QString("INSERT INTO journal VALUES(NULL,'%1');")
                          .arg(jName);
        QSqlQuery qry;
        qry.exec(querystring);
        QMessageBox success;
        success.setText("Journal was successfully added!");
        success.setWindowTitle("Journal Added!");
        success.exec();
    }
    else
    {
       QMessageBox warning;
       warning.setText("Journal name is blank, please fill in the information"
                +   QString(" and try again!This transaction will not be saved!"));
       warning.setWindowTitle("No Journal Name");
       warning.exec();
    }
}
