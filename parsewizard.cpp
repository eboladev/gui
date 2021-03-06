#include "parsewizard.h"
#include "ui_parsewizard.h"
#include "QLabel"
#include "QStandardItem"
#include "QHeaderView"
#include "addauthor.h"
#include "QList"
#include "QDebug"
#include"QMessageBox"
#include "QPushButton"
#include "QFileDialog"
#include "QFile"
#include "QSignalMapper"
#include "editparsedreferences.h"
#include "QProcess"
#include "utility.h"
#include "QToolButton"

QString cJourn;

typedef QList< QStandardItem* > StandardItemList;

parseWizard::parseWizard(QWidget *parent) :
    QWizard(parent),
    ui(new Ui::parseWizard)
{
    proxtotal = 0;
    flagDone = false;

    ui->setupUi(this);


    //set up authors table
    ui->authorTable->alternatingRowColors();
    model = new QStandardItemModel(0,2);
    model->setHeaderData(0,Qt::Horizontal,tr("Author"));
    model->setHeaderData(1,Qt::Horizontal,tr("Affiliation"));
    ui->authorTable->setModel(model);

    //author set up ends here

    on_lineEdit_2_textChanged("");

    //
      ui->authorTable->resizeColumnsToContents();
      QHeaderView* header = ui->authorTable->horizontalHeader();

      for(int i=0;i<header->count();i++)
         header->setSectionResizeMode(i,QHeaderView::Stretch);

      ui->authorTable->setHorizontalHeader(header);
      ui->authorTable->resizeRowsToContents();

      setOption( QWizard::DisabledBackButtonOnLastPage, true );




  QWidget *central = new QWidget;

  QVBoxLayout *layout = new QVBoxLayout(central);
  ui->scrollArea->setWidget(central);
  ui->scrollArea->setWidgetResizable(true);


  QSqlQueryModel *parseModel = new QSqlQueryModel;
  QString pQuery =
          QString("SELECT name,path FROM parser");

  parseModel->setQuery(pQuery);

  ui->parserCombo->setModel(parseModel);

  parseMap.clear();
  for(int i = 0; i < parseModel->rowCount(); i++)
  {
      parseMap[parseModel->data(parseModel->index(i,0)).toString()] =
                        parseModel->data(parseModel->index(i,1)).toString();
  }

}

parseWizard::~parseWizard()
{
    delete ui;
}


void parseWizard::on_lineEdit_2_textChanged(const QString &arg1)
{
    sqlmodel = new QSqlQueryModel;
    QString wcard = ui->lineEdit_2->text();
    QString queryString = QString("SELECT * FROM JOURNAL WHERE NAME LIKE '\%%1\%'").arg(wcard);
    sqlmodel->setQuery(queryString);

    sqlmodel->setHeaderData(1,Qt::Horizontal,tr("Journal"));
    ui->journalTable->setModel(sqlmodel);

    ui->journalTable->resizeColumnsToContents();
    QHeaderView* header = ui->journalTable->horizontalHeader();

    header->setStretchLastSection(true);

    ui->journalTable->setHorizontalHeader(header);
    ui->journalTable->setColumnHidden(0,true);


}

void parseWizard::on_journalTable_clicked(const QModelIndex &index)
{
    QString data = ui->journalTable->model()->data(ui->journalTable->model()->index(index.row(),0)).toString();
    cJourn=data;
    QSqlQueryModel *vol = new QSqlQueryModel;
    QString volquery = QString("SELECT volume FROM journal_volume where journal_id=%1").arg(data);
    vol->setQuery(volquery);
    ui->comboBox->setModel(vol);
}

void parseWizard::on_comboBox_currentTextChanged(const QString &arg1)
{
    QString volData = ui->comboBox->currentText();
    QSqlQueryModel *issue = new QSqlQueryModel;
    QString issuequery = QString("SELECT issue FROM journal_issue where journal_id=%1 and volume=%2").arg(cJourn).arg(volData);
    issue->setQuery(issuequery);
    ui->comboBox_2->setModel(issue);
}

void parseWizard::on_pushButton_3_clicked()
{
    ui->textEdit->clear();
}

void parseWizard::on_pushButton_2_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,"Open raw reference file",".","Reference Files (*.in)");
    if( filename.isEmpty())
        return;

    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly|QIODevice::Text))
    return;

   ui->textEdit->clear();
  QTextStream in(&file);

   while(!in.atEnd())
   {
      QString line = in.readLine();
        ui->textEdit->append(line);
   }

}
void parseWizard::viewPage(int index)
{
    editIndex=index;
    ui->textEdit_2->setText(Utility::accumulate(index,vvqs));
    ui->label_14->setText(QString("Errors for current output: %1")
    .arg(errCount[editIndex]));
}

void parseWizard::on_parseWizard_currentIdChanged(int id)
{
    if(id == 2 && ui->lineEdit->text().trimmed() == ""){
        QMessageBox warning;
        warning.setText("Cannot proceed without filling in Article Name!");
        warning.setWindowTitle("Blank Article Name!");
        warning.exec();
        this->back();

    }

    bool flag = true;

    if(id == 3 && cJourn == ""){

        QMessageBox warning;
        warning.setText("Cannot proceed without journal,volume and issue selected!");
        warning.setWindowTitle("No Journal, Volume, or Issue Data selected!");
        warning.exec();
        this->back();
        flag = false;
    }
    if(id == 3 &&
         (ui->comboBox->currentText() == "" ||
          ui->comboBox_2->currentText()== "") &&
          flag == true){

        QMessageBox warning;
        warning.setText("Cannot proceed without journal,volume and issue selected!");
        warning.setWindowTitle("No Journal, Volume, or Issue Data selected!");
        warning.exec();
        this->back();
    }

    bool parseFlag = true;

    if(id == 4)
    {
        if(ui->textEdit->toPlainText().trimmed() == ""){
            parseFlag=false;
            this->back();
            QMessageBox warning;
            warning.setText("Cannot proceed without reference data to parse!");
            warning.setWindowTitle("Blank Reference Text!");
            warning.exec();

        }

    }

    if(id == 4 && parseFlag)
    {
       delete ui->scrollArea->layout();
       QString finishText = ui->textEdit->toPlainText();
       QFile file("temp.txt");
       file.open(QIODevice::ReadWrite|QIODevice::Truncate|QIODevice::Text);
       QTextStream out(&file);
       out<<finishText;
       file.close();

       QString program = parseMap[ui->parserCombo->currentText()];
       QStringList arguments;

       arguments << QString(QApplication::applicationDirPath().append("/temp.txt"));

       QProcess *process = new QProcess(this);
       process->start(program,arguments);
       QMessageBox qmsg;

       if(finishText=="")
           qmsg.setText("PARSING DIDN'T START, BLANK INPUT!!!");
       else if(!process->waitForStarted())
           qmsg.setText("PARSING DIDN'T START");
       else if(!process->waitForFinished())
           qmsg.setText("PARSING DIDN'T FINISH");
       else{
           qmsg.setText("PARSING COMPLETE!YOU MAY NOW VIEW OUTPUT!");
            flagDone=true;
        }

       qmsg.exec();
       QString output(process->readAllStandardOutput());
        qDebug()<<output;


        QTextStream qts(&output);
        QVector <QString> vq;
        vq.clear();
        vvqs.clear();
        int count = 0;
        int outputCount=0;


        while(!qts.atEnd())
        {
            QString line = qts.readLine();
            count = 0;
            if(line=="****")
            {
                outputCount++;
                while(line!="####")
                {
                    line = qts.readLine();
                    if(line=="####") break;
                    vq.push_back("");
                    vq[vq.size()-1].append(line);
                    vq[vq.size()-1].append("\n");
                    count++;
                }


             }
            if(vq.size()>=1)
            vvqs.push_back(vq);
            vq.clear();
          }

        qpb.clear();
        errCount.clear();
        qpb.resize(vvqs.size());
        errCount.resize(vvqs.size());
        QSignalMapper *mapper = new QSignalMapper(this);
        QWidget *central = new QWidget;

        QVBoxLayout *layout = new QVBoxLayout(central);


        for(int i=0;i< qpb.size();i++)
        {
            qpb[i]=new QPushButton;
            qpb[i]->setText(QString("Output %1").arg(i+1));
            layout->addWidget(qpb[i]);
            connect(qpb[i],SIGNAL(clicked()),mapper,SLOT(map()));

            mapper->setMapping(qpb[i],i);
        }

        ui->scrollArea->setWidget(central);
        ui->scrollArea->setWidgetResizable(true);
        connect(mapper,SIGNAL(mapped(int)),this,SLOT(viewPage(int)));
        connect(ui->pushButton_4,SIGNAL(clicked()),this,SLOT(editPage()));





    }

 }

void parseWizard::editPage()
{
    editwidget=new EditParsedReferences;
    editwidget->setWindowTitle("Edit Parsed References");

    qgrid = new QGridLayout;
    qDebug() << editIndex;
    vqle1.clear();
    vqle2.clear();
    vqtb.clear();
    vAddedRow.clear();

    QSignalMapper *mapper = new QSignalMapper(this);

    if(vvqs.empty())
    {
       QMessageBox warning;
       warning.setText("No Output Selected!");
       warning.setWindowTitle("No Output Selected!");
       warning.exec();
       return;
    }

    for(int i=0;i<vvqs[editIndex].size();i++)
    {
        QString line = vvqs[editIndex][i];
        int g = line.indexOf(':');

        QLineEdit *qle = new QLineEdit;
        QLineEdit *lll = new QLineEdit;
        lll->setFixedWidth(150);

        QString before = line.left(g);
        QString after = line.mid(g+1);
        lll->setText(before.trimmed());
        qle->setText(after.trimmed());

        QToolButton *qtb = new QToolButton;
        qtb->setIcon(QIcon(QPixmap(":images/minus.png")));
        qtb->setIconSize(QSize(24,24));


        vqle1.push_back(lll);
        vqle2.push_back(qle);
        vqtb.push_back(qtb);
        vAddedRow.push_back(false);
        qgrid->addWidget(lll,i+1,0);
        qgrid->addWidget(qle,i+1,1);
        qgrid->addWidget(qtb,i+1,2);
        connect(lll,SIGNAL(editingFinished()),this,SLOT(addError()));
        connect(qle,SIGNAL(editingFinished()),this,SLOT(addError()));
        connect(qtb,SIGNAL(clicked()),mapper,SLOT(map()));
        mapper->setMapping(qtb,i+1);

    }
    connect(mapper,SIGNAL(mapped(int)),this,SLOT(deleteRow(int)));

    ok = new QPushButton;
    ok->setText("Save Changes");
    cancel = new QPushButton;
    cancel->setText("Ignore Changes");

    addrow = new QPushButton;
    addrow->setText("Add Row");

    ok->setFixedWidth(125);
    cancel->setFixedWidth(125);
    addrow->setFixedWidth(125);

    qgrid->addWidget(ok);
    qgrid->addWidget(cancel);
    qgrid->addWidget(addrow);


    editwidget->setScrollArea(qgrid);

    connect(ok,SIGNAL(clicked()),editwidget,SLOT(accept()));
    connect(cancel,SIGNAL(clicked()),editwidget,SLOT(reject()));
    connect(addrow,SIGNAL(clicked()),this,SLOT(addRow()));

    connect(editwidget,SIGNAL(accepted()),this,SLOT(commitError()));
    connect(editwidget,SIGNAL(rejected()),this,SLOT(eraseError()));
    viewPage(editIndex);

    if(!errCount.empty())
    editwidget->changeError(errCount[editIndex]);


    editwidget->exec();
}


void parseWizard::commitError()
{
    errCount[editIndex]+=proxtotal;
    proxtotal=0;
    vvqs[editIndex].clear();
    for(int i=0;i<vqle1.size();i++)
    {
        if(vqle1[i]->text() != "-1"){
            QString str1 = vqle1[i]->text();
            QString str2= vqle2[i]->text();
            vvqs[editIndex].push_back(str1.trimmed().append(" : ")
                                .append(str2).trimmed());
        }
    }
    int totError = Utility::sumOverErrors(errCount);
    QString totErrorString = QString("%1").arg(totError);
    ui->label_17->setText("Total Errors: " + totErrorString);
    viewPage(editIndex);
}

void parseWizard::addError()
{
    QLineEdit *lineedit = dynamic_cast<QLineEdit*>(sender());
    if(lineedit->isModified()==true){
        if(modifiedLine.find(lineedit)==modifiedLine.end()){
            proxtotal++;
            editwidget->changeError(proxtotal);
            modifiedLine.insert(lineedit);
   }
   }
 }

void parseWizard::eraseError()
{
    proxtotal=0;
}

void parseWizard::on_parseWizard_accepted()
{
    if(vvqs.empty())
    {
        QMessageBox warning;
        warning.setText("No citations were parsed! This transaction will not be saved!");
        warning.setWindowTitle("No citations parsed!");
        warning.exec();
    }
    else{
        if(flagDone){
            QString articleName = ui->lineEdit->text();
            QString journData = cJourn;
            QString volData = ui->comboBox->currentText();
            QString issData = ui->comboBox_2->currentText();
            QString parseVer = ui->parserCombo->currentText();
            QString pages = ui->lineEdit_4->text();

            QString querystring =
                  QString("INSERT INTO article VALUES(NULL,'%1','%2','%3',%4,%5,%6);")
                 .arg(articleName).arg(pages).arg(parseVer).arg(journData).arg(volData)
                    .arg(issData);

            QSqlQuery query;
            query.exec(querystring);

               QSqlQuery selQuery;

               selQuery.exec("SELECT MAX(id) from article");
              selQuery.next();
               int id=selQuery.value(0).toInt();

               for(int i=0;i<vvqs.size();i++)
               {
                   QString content = Utility::accumulate(i,vvqs);
                 QString querystring =
                         QString("INSERT INTO citation VALUES(NULL,'%1',%2,%3);")
                         .arg(content).arg(errCount[i]).arg(id);
                   query.exec(querystring);
               }

               int row = ui->authorTable->model()->rowCount();
               int col = ui->authorTable->model()->columnCount();


               for(int i=0;i<row;i++){
                   QModelIndex authqmi = ui->authorTable->model()->index(i,0);
                   QModelIndex affilqmi = ui->authorTable->model()->index(i,1);
                   QString author = ui->authorTable->model()->data(authqmi)
                                    .toString();

                   QString affiliation = ui->authorTable->model()->data(affilqmi)
                                        .toString();
                   QString insquery =
                           QString("INSERT INTO author values(NULL,'%1','%2',%3)")
                           .arg(author).arg(affiliation).arg(id);

                   query.exec(insquery);
                }

           }
    }
    flagDone=false;
}

void parseWizard::deleteRow(int row){
    QLayoutItem *item = qgrid->itemAtPosition(row,0);
    QLineEdit *lineedit = dynamic_cast<QLineEdit*> (item->widget());

    if(vAddedRow[row-1] == false)
        proxtotal++;
    else
        proxtotal--;

    if(modifiedLine.find(lineedit) != modifiedLine.end())
        proxtotal--;

    item->widget()->hide();
    item = qgrid->itemAtPosition(row,1);

    lineedit = dynamic_cast<QLineEdit*> (item->widget());

    if(modifiedLine.find(lineedit) != modifiedLine.end())
        proxtotal--;

    item->widget()->hide();
    item = qgrid->itemAtPosition(row,2);
    item->widget()->hide();

    editwidget->changeError(proxtotal);

    vqle1[row-1]->setText("-1");
    vqle2[row-1]->setText("-1");
}

void parseWizard::on_addAuthor_clicked()
{

    AddAuthor *aA = new AddAuthor;
     int retcode = aA->exec();

     if(retcode==1){

           QStandardItem *auth = new QStandardItem(QString(aA->author));
           QStandardItem *affil = new QStandardItem(QString(aA->affiliation));

         if(aA->author.trimmed() != "" && aA->affiliation.trimmed() != "")
               model->appendRow( StandardItemList() << auth << affil );
         else
         {
             QMessageBox warning;
             warning.setText("One of the fields were left blank! Please try again! This transaction will not be saved!");
             warning.setWindowTitle("No Journal Selected");
             warning.exec();
         }
     }


    connect(aA,SIGNAL(destroyed()),aA,SLOT(deleteLater()));
}

void parseWizard::on_deleteAuthor_clicked()
{
    QModelIndex index = ui->authorTable->currentIndex();

    if(index.row() != -1)
        model->removeRow(index.row());
    else
    {
        QMessageBox warning;
        warning.setText("No author selected to be removed! Please select an author then try again!");
        warning.setWindowTitle("No Author Selected");
        warning.exec();
    }
}


void parseWizard::addRow(){
    proxtotal++;
    QLineEdit *qle1 = new QLineEdit;
    QLineEdit *qle2 = new QLineEdit;
    qle1->setFixedWidth(150);

    QToolButton *qtb = new QToolButton;
    qtb->setIcon(QIcon(QPixmap(":images/minus.png")));
    qtb->setIconSize(QSize(24,24));

    vqle1.push_back(qle1);
    vqle2.push_back(qle2);
    vqtb.push_back(qtb);
    vAddedRow.push_back(true);

    qgrid->addWidget(qle1,vqle1.size(),0);
    qgrid->addWidget(qle2,vqle2.size(),1);
    qgrid->addWidget(qtb,vqtb.size(),2);
    qgrid->removeWidget(addrow);
    qgrid->addWidget(ok);
    qgrid->addWidget(cancel);
    qgrid->addWidget(addrow);

    editwidget->changeError(proxtotal);

      QSignalMapper *mapper = new QSignalMapper(this);


      connect(qtb,SIGNAL(clicked()),mapper,SLOT(map()));
      mapper->setMapping(qtb,vqtb.size());




    connect(mapper,SIGNAL(mapped(int)),this,SLOT(deleteRow(int)));

}
