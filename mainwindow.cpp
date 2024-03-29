#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QObject>
#include "QStandardItem"
#include <QMessageBox>
#include <QStringList>
#include <QStringListModel>
#include <iostream>
#include <vector>
#include <sstream>
#include "csvreader.h"
#include "lsregression.h"
#include "residual.h"

extern csvReader csvreader;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)

{
    setupUi(this);
}


MainWindow::~MainWindow()
{
//    delete ui;
}


void MainWindow::on_exitButton_clicked()
{
    this->close();
}


void MainWindow::putdata()
{
    pri_csvreader = csvreader;
    this->residualtable->setVisible(false);
    this->datatable->setVisible(true);
    QStandardItemModel *model;
    model = new QStandardItemModel(pri_csvreader.getNRows() + 1, pri_csvreader.getNCols() + 1);
    this->datatable->setModel(model);

    // Set header (title) for the data table.
    model->setHeaderData(0, Qt::Horizontal, (" "));
    for (int i = 1; i < pri_csvreader.getNCols() + 1; i++)
    {
        QString str = QString::fromStdString(pri_csvreader.getTitleList()[i - 1]);
        model->setHeaderData(i, Qt::Horizontal, (str));
    }

    // Set data for each cell in data table.
    arma::mat matrix = pri_csvreader.getDataMatrix();
    for (int i = 1; i < pri_csvreader.getNRows() + 1; i++)
    {
        QModelIndex rownum = model->index(i, 0, QModelIndex());
        model->setData(rownum, i);
        for (int j = 1; j < pri_csvreader.getNCols() + 1; j++)
        {
            QModelIndex index = model->index(i, j, QModelIndex());
            model->setData(index, matrix(i - 1, j - 1));
        }
    }

    // Set ckeckbox.
    for (int i = 1; i < pri_csvreader.getNCols() + 1; i++)
    {
        QStandardItem *Item = new QStandardItem();
        Item->setCheckable(true);
        Item->setCheckState(Qt::Unchecked);
        model->setItem(0, i, Item);
    }

//    this->datatable->setColumnWidth(0, 40);
    QHeaderView* headerView = this->datatable->verticalHeader();
    headerView->setHidden(true);

    // Resize
    this->datatable->resizeColumnsToContents();

    // Set the table become no edit mode. (Ban users from editing the table)
    this->datatable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // Show
    this->tab2->setVisible(false);
    this->tab2->setEnabled(false);
    this->tabWidget->setCurrentIndex(0);
    this->datatable->show();

    /* Connect itemchanged signal to slots.
    *This will listen to the checkbox in table.*/
    connect(model, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(col_select_test()));
}


void MainWindow::col_select_test()
{
    if (methodActivated)
    {
        setxbutton->setEnabled(true);
        setybutton->setEnabled(true);

    }
    else col_set = true;
}


void MainWindow::row_select_test()
{
       deletebutton->setEnabled(true);
}


void MainWindow::on_setxbutton_clicked()
{
    isXset = true;
    dataX.clear();
    bool nomistake = true;

    QStandardItemModel *_model = static_cast<QStandardItemModel*>(this->datatable->model());

    // Button set part.
    if (isYset)
    {
        setSbutton->setEnabled(true);
        cookmeasure->setCheckable(true);
        execButton->setEnabled(true);
    }

    if (isrobust && isYset)
    {
        setTbutton->setEnabled(true);
    }

    // Get the changed checkbox.

    for (int i = 1; i < pri_csvreader.getNCols() + 1; i++)
    {
        QStandardItem *Item = _model->item(0, i);
        if (Item->checkState() == Qt::Checked)
        {
           dataX.push_back(i - 1);
           Item->setCheckState(Qt::Unchecked);
        }
    }

    // for simple
    if (dataX.size() == 0)
    {
        QMessageBox::warning(this,tr("Warning"),("Please select at least one colummn."), QMessageBox::Yes);
    }
    if (methodtype == 1 && dataX.size() != 1)
    {
        QMessageBox::warning(this,tr("Warning"),("Please select only one X for simple linear regression."), QMessageBox::Yes);
        nomistake = false;
    }
    if (methodtype == 2 && dataX.size() == 1)
    {
        QMessageBox::warning(this,tr("Warning"),("Please use simple least square regression if only one X."), QMessageBox::Yes);
        nomistake = false;
    }


    // Background change.
    if (nomistake)
    {
        int datasize = dataX.size();
        for (int i = 0; i < datasize; i++)
        {
            int colnum = dataX[i];
            for (int j = 1; j < pri_csvreader.getNRows() + 1; j++)
            {
                QStandardItem *item = _model->item(j, colnum + 1);
                item->setForeground(QBrush(QColor(0, 0, 255)));
            }
        }
    }


}


void MainWindow::on_setybutton_clicked()
{
    isYset = true;
    // Button set part.
    if (isXset)
    {
        setSbutton->setEnabled(true);
        execButton->setEnabled(true);
    }

    if (isrobust && isXset)
    {
        setTbutton->setEnabled(true);
    }

    // Get the changed checkbox
    QStandardItemModel *_model = static_cast<QStandardItemModel*>(this->datatable->model());
    std::vector<int> tempvec;
    for (int i = 1; i < pri_csvreader.getNCols() + 1; i++)
    {
        QStandardItem *Item = _model->item(0, i);
        if (Item->checkState() == Qt::Checked)
        {
           tempvec.push_back(i - 1);
           Item->setCheckState(Qt::Unchecked);
        }
    }

    if (tempvec.size() != 1)
    {
        QMessageBox::warning(this,tr("Warning"),("Invalid operation, Please select one and only one Y for regression."), QMessageBox::Yes);
    }
    else
    {
        dataY = tempvec[0];

        // Background change.
        for (int j = 1; j < pri_csvreader.getNRows() + 1; j++)
        {
            QStandardItem *item = _model->item(j, dataY + 1);
            item->setForeground(QBrush(QColor(255, 0, 0)));
        }
    }
}


void MainWindow::on_methodcombobox_activated(const QString &arg1)
{
    // Background reset.
    QStandardItemModel *_model = static_cast<QStandardItemModel*>(this->datatable->model());
    for (int i = 1; i < pri_csvreader.getNRows() + 1; i++)
    {
        for (int j = 1; j < pri_csvreader.getNCols() + 1; j++)
        {
            QStandardItem *item = _model->item(i, j);
            item->setForeground(QBrush(QColor(0, 0, 0)));
        }
    }

    // Three method and default text
    if (arg1 == "Please select")
    {
        this->datatable->setVisible(true);
        this->residualtable->setVisible(false);
        this->textBrowser->clear();
        this->graph->setHidden(true);
        this->tabWidget->setCurrentIndex(0);
        cookmeasure->setCheckable(false);
        setxbutton->setEnabled(false);
        setybutton->setEnabled(false);
        setSbutton->setEnabled(false);
        execButton->setEnabled(false);
        setTbutton->setEnabled(false);
        deletebutton->setEnabled(false);
    }

    if (arg1 == "Simple Least Square Regression")
    {
        methodtype = 1;
        this->datatable->setVisible(true);
        this->residualtable->setVisible(false);
        this->textBrowser->clear();
        this->graph->setHidden(true);
        this->tabWidget->setCurrentIndex(0);

        // Set method data
        pri_ls_simple.setDataSource(pri_csvreader);

        /* This part is to ensure when the user change method during operation,
         * all the button will be disabled again.                               */
        isXset = isYset = false;
        cookmeasure->setCheckable(false);
        setxbutton->setEnabled(false);
        setybutton->setEnabled(false);
        setSbutton->setEnabled(false);
        execButton->setEnabled(false);
        setTbutton->setEnabled(false);
        deletebutton->setEnabled(false);

        methodActivated = true;
        if (col_set){
            setxbutton->setEnabled(true);
            setybutton->setEnabled(true);
        }

    }
    if (arg1 == "Multiple Least Square Regression")
    {
        methodtype = 2;
        this->datatable->setVisible(true);
        this->residualtable->setVisible(false);
        this->textBrowser->clear();
        this->graph->setHidden(true);
        this->tabWidget->setCurrentIndex(0);

        // Set method data
        pri_ls_multi.setDataSource(pri_csvreader);

        /* This part is to ensure when the user change method during operation,
         * all the button will be disabled again.                               */
        isXset = isYset = false;
        cookmeasure->setCheckable(false);
        setxbutton->setEnabled(false);
        setybutton->setEnabled(false);
        setSbutton->setEnabled(false);
        execButton->setEnabled(false);
        setTbutton->setEnabled(false);
        deletebutton->setEnabled(false);

        methodActivated = true;

        if (col_set){
            setxbutton->setEnabled(true);
            setybutton->setEnabled(true);
        }

    }
    if (arg1 == "Robust Regression")
    {
        methodtype = 3;
        this->datatable->setVisible(true);
        this->residualtable->setVisible(false);
        this->textBrowser->clear();
        this->graph->setHidden(true);
        this->tabWidget->setCurrentIndex(0);

        // Set method data
        pri_ls_rob.setDataSource(pri_csvreader);
        pri_robregression.setLSR(pri_ls_rob);

        /* This part is to ensure when the user change method during operation,
         * all the button will be disabled again.                               */
        isXset = isYset = false;
        cookmeasure->setCheckable(false);
        setxbutton->setEnabled(false);
        setybutton->setEnabled(false);
        setSbutton->setEnabled(false);
        execButton->setEnabled(false);
        setTbutton->setEnabled(false);
        deletebutton->setEnabled(false);

        methodActivated = true;
        isrobust = true;

        if (col_set){
            setxbutton->setEnabled(true);
            setybutton->setEnabled(true);
        }

    }
}


bool MainWindow::isdouble(std::string s)
{
        std::stringstream sin(s);
        double t;
        char p;
        if (!(sin >> t))
        {
            return false;
        }
        if (sin >> p){
            return false;
        }
        else
        {
             return true;
        }
}


void MainWindow::on_execButton_clicked()
{
    cookmeasure->setCheckable(true);
    residualbutton->setEnabled(true);
//    this->datatable->setVisible(false);
    // Simple Least Square
    if (methodtype == 1)
    {
        this->pri_ls_simple.set(dataX, dataY);
        this->pri_ls_simple.setSignificance(significance_num);
        this->pri_ls_simple.solve();
        std::vector<std::vector<std::string>> simsummary;
        std::vector<std::string> simtext;
        this->pri_ls_simple.printSummary(simsummary, simtext);
        putsummary(simsummary, simtext);


    }

//     Multiple Least Square
    if (methodtype == 2)
    {
        this->pri_ls_multi.set(dataX, dataY);
        this->pri_ls_multi.setSignificance(significance_num);
        this->pri_ls_multi.solve();
        std::vector<std::vector<std::string>> multisummary;
        std::vector<std::string> multitext;
        this->pri_ls_multi.printSummary(multisummary, multitext);
        putsummary(multisummary, multitext);
    }
    // Robust
    if (methodtype == 3)
    {
        this->pri_ls_rob.set(dataX, dataY);
        this->pri_ls_rob.setSignificance(significance_num);
        this->pri_ls_rob.solve();

        this->pri_robregression.setLSR(pri_ls_rob);
        this->pri_robregression.setT(t_num);
        std::vector<std::vector<std::string>> robsummary;
        std::vector<std::string> robtext;
        this->pri_robregression.solve();
        this->pri_robregression.printSummary(robsummary, robtext);
        putsummary(robsummary, robtext);
    }

}


void MainWindow::on_cookmeasure_clicked()
{
    iscookmeasure = !iscookmeasure;
}


void MainWindow::on_setSbutton_clicked()
{
    std::string s = this->significanceedit->text().toStdString();
    if (isdouble(s))
    {
        std::stringstream ss(s);
        double temp;
        ss >> temp;
        if (temp < 0 || temp > 1)
        {
            QMessageBox::warning(this,tr("Warning"),("Please input a number in (0, 1)."), QMessageBox::Yes);
        }
        else
        {
            ss >> significance_num;
        }
    }
    else
    {
        QMessageBox::warning(this,tr("Warning"),("Please input a number."), QMessageBox::Yes);
    }
    this->significanceedit->clear();

}

void MainWindow::on_setTbutton_clicked()
{
    std::string s = this->huberedit->text().toStdString();
    if (isdouble(s))
    {
        std::stringstream ss(s);
        double temp;
        ss >> temp;
        if (temp < 0)
        {
            QMessageBox::warning(this,tr("Warning"),("Please input a number greater than 0."), QMessageBox::Yes);
        }
        else
        {
            ss >> t_num;
        }

    }
    else
    {
        QMessageBox::warning(this,tr("Warning"),("Please input a number."), QMessageBox::Yes);
    }
    this->huberedit->clear();

}


void MainWindow::putsummary(std::vector<std::vector<std::string> > summary, std::vector<std::string> text)
{
    int size1 = summary.size();
    for (int i = 0; i < size1; i++)
    {
        int size2 = summary[i].size();

        std::string str;

        for (int j = 0; j < size2; j++)
        {
            str += summary[i][j] + "\t";
        }

        QString qstr = QString::fromStdString(str);
        this->textBrowser->append(qstr);
    }
    this->textBrowser->append(" ");
    int size3 = text.size();

    for (int i = 0; i <size3; i++)
    {
        std::string textstr;
        textstr += text[i];
        QString qstring = QString::fromStdString(textstr);
        this->textBrowser->append(qstring);
    }
    this->textBrowser->append(" ");
    this->textBrowser->append(" ");
    this->textBrowser->append(" ");
}


void MainWindow::putResidualsummary(std::vector<std::vector<std::string> > & analysis)
{
    this->datatable->setVisible(false);
    this->residualtable->setVisible(true);
    this->residualtable->clearSpans();

    outlierIndex.clear();
    nonOutlier.clear();

    QStandardItemModel *model;
    residualRow = analysis.size();
    residualCol = analysis[0].size();
    model = new QStandardItemModel(residualRow - 1, residualCol + 1);
    this->residualtable->setModel(model);

    // Set header (title) for the data table.
    model->setHeaderData(0, Qt::Horizontal, (" "));
    for (int i = 1; i < residualCol + 1; i++)
    {
        QString str = QString::fromStdString(analysis[0][i - 1]);
        model->setHeaderData(i, Qt::Horizontal, (str));
    }

    // Set data for each cell in data table.
    for (int i = 1; i < residualRow; i++)
    {
        // Outlier Judge
        if (analysis[i][residualCol - 1] == std::string("Yes"))
        {
            outlierIndex.push_back(i - 1);
        }
        else if (analysis[i][residualCol - 1] == std::string("No"))
        {
            nonOutlier.push_back(i - 1);
        }

        for (int j = 1; j < residualCol + 1; j++)
        {
            QString str = QString::fromStdString(analysis[i][j - 1]);
            QModelIndex index = model->index(i - 1, j, QModelIndex());
            model->setData(index, str);
        }
    }

    // Set ckeckbox.
    for (int i = 0; i < residualRow - 1; i++)
    {
        QStandardItem *Item = new QStandardItem();

        Item->setCheckable(true);
        Item->setCheckState(Qt::Unchecked);
        model->setItem(i, 0, Item);
    }

    QHeaderView* headerView = this->residualtable->verticalHeader();
    headerView->setHidden(true);

    // Resize
    this->residualtable->resizeColumnsToContents();

    // Set the table become no edit mode. (Ban users from editing the table)
    this->residualtable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Show
    this->residualtable->show();

    /* Connect itemchanged signal to slots.
    *This will listen to the checkbox in table.*/
    connect(model, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(row_select_test()));

}


void MainWindow::on_residualbutton_clicked()
{
    this->graph->clearGraphs();
    residualbutton->setEnabled(false);

    // Simple Least Square
    if (methodtype == 1)
    {
        arma::mat linedata = this->pri_ls_simple.getbetaHat();
        double data0 = linedata(0);
        double data1 = linedata(1);

        // New Summary
        std::vector<std::vector<std::string> > analysis;
        this->pri_ls_simple.ResidualAnalysis(iscookmeasure, analysis);
        putResidualsummary(analysis);

        // Plot scatter points
        arma::mat X = this->pri_ls_simple.getX();
        arma::mat Y = this->pri_ls_simple.getY();
        plotScatter(X, Y, pri_ls_simple);

        // Plot line
        plotRegressionLine(data0, data1);

        // Stack operation.
        this->stack_ls.setInitial( & pri_ls_simple);

    }

    // Multiple Least Square
    if (methodtype == 2)
    {

        // New summary
        std::vector<std::vector<std::string> > multianalysis;
        this->pri_ls_multi.ResidualAnalysis(iscookmeasure, multianalysis);
        putResidualsummary(multianalysis);

        // Stack operation.
        this->stack_multi.setInitial( & pri_ls_multi);

    }

    // Robust
    if (methodtype == 3)
    {

        // New summary
        std::vector<std::vector<std::string> > robanalysis;
        this->pri_robregression.ResidualAnalysis(iscookmeasure, robanalysis);
        putResidualsummary(robanalysis);

        // Check if it can plot
        if (dataX.size() == 1)
        {
            // Plot scatter points
            arma::mat robX = this->pri_robregression.getX();
            arma::mat robY = this->pri_robregression.getY();
            plotScatter(robX, robY, pri_robregression);

            // Plot line
            arma::mat robdata = this->pri_robregression.getbetaHat();
            double robdata0 = robdata(0);
            double robdata1 = robdata(1);
            plotRegressionLine(robdata0, robdata1);
        }

        // Stack operation.
        this->stack_rob.setInitial( & pri_ls_rob);
    }
}

void MainWindow::plotScatter(arma::mat & X, arma::mat & Y, LSregression lsregression)
{
    this->graph->setHidden(false);
    int nonOutliersize = nonOutlier.size();
    int outliersize = outlierIndex.size();
    QVector<double> x0(nonOutliersize), y0(nonOutliersize);
    QVector<double> x1(outliersize), y1(outliersize);

    // For non-oulier
    for (int i = 0; i < nonOutliersize; i++)
    {
        x0[i] = X(nonOutlier[i], 1);
        y0[i] = Y(nonOutlier[i]);
    }

    // For outliers
    for (int i = 0; i < outliersize; i++)
    {
        x1[i] = X(outlierIndex[i], 1);
        y1[i] = Y(outlierIndex[i]);
    }

    this->graph->xAxis->setLabel("X");
    this->graph->yAxis->setLabel("Y");
    double Xrange = lsregression.Xmax() - lsregression.Xmin();
    double Yrange = lsregression.Ymax() - lsregression.Ymin();
    this->graph->xAxis->setRange(lsregression.Xmin() - Xrange * 0.2, lsregression.Xmax() + Xrange * 0.2);
    this->graph->yAxis->setRange(lsregression.Ymin() - Yrange * 0.2, lsregression.Ymax() + Yrange * 0.2);

    // For non-outlier
    this->graph->addGraph();
    this->graph->graph(0)->setPen(QColor(50, 50, 50, 255));
    this->graph->graph(0)->setLineStyle(QCPGraph::lsNone);
    this->graph->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 3));
    this->graph->graph(0)->setData(x0, y0);
    this->graph->graph(0)->setName("Non-outlier data points");

    // For outliers
    this->graph->addGraph();
    this->graph->graph(1)->setPen(QColor(255, 0, 0));
    this->graph->graph(1)->setLineStyle(QCPGraph::lsNone);
    this->graph->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 3));
    this->graph->graph(1)->setData(x1, y1);
    this->graph->graph(1)->setName("Potential outliers");

    this->tabWidget->setCurrentIndex(1);
}


void MainWindow::plotScatter(arma::mat & X, arma::mat & Y, robustregression lsregression)
{
    this->graph->setHidden(false);
    int nonOutliersize = nonOutlier.size();
    int outliersize = outlierIndex.size();
    QVector<double> x0(nonOutliersize), y0(nonOutliersize);
    QVector<double> x1(outliersize), y1(outliersize);

    // For non-oulier
    for (int i = 0; i < nonOutliersize; i++)
    {
        x0[i] = X(nonOutlier[i], 1);
        y0[i] = Y(nonOutlier[i]);
    }

    // For outliers
    for (int i = 0; i < outliersize; i++)
    {
        x1[i] = X(outlierIndex[i], 1);
        y1[i] = Y(outlierIndex[i]);
    }

    this->graph->xAxis->setLabel("X");
    this->graph->yAxis->setLabel("Y");
    double Xrange = lsregression.Xmax() - lsregression.Xmin();
    double Yrange = lsregression.Ymax() - lsregression.Ymin();
    this->graph->xAxis->setRange(lsregression.Xmin() - Xrange * 0.2, lsregression.Xmax() + Xrange * 0.2);
    this->graph->yAxis->setRange(lsregression.Ymin() - Yrange * 0.2, lsregression.Ymax() + Yrange * 0.2);

    // For non-outlier
    this->graph->addGraph();
    this->graph->graph(0)->setPen(QColor(50, 50, 50, 255));
    this->graph->graph(0)->setLineStyle(QCPGraph::lsNone);
    this->graph->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 3));
    this->graph->graph(0)->setData(x0, y0);
    this->graph->graph(0)->setName("Non-outlier data points");

    // For outliers
    this->graph->addGraph();
    this->graph->graph(1)->setPen(QColor(255, 0, 0));
    this->graph->graph(1)->setLineStyle(QCPGraph::lsNone);
    this->graph->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 3));
    this->graph->graph(1)->setData(x1, y1);
    this->graph->graph(1)->setName("Potential outliers");

    this->tabWidget->setCurrentIndex(1);
}


void MainWindow::plotRegressionLine(double beta0, double beta1)
{
    QVector<double> x2(200), y2(200);

    for (int i = 0; i < 200; i++)
    {
        x2[i] = i - 50;
        y2[i] = beta1 * x2[i] + beta0;
    }
    this->graph->addGraph();
    this->graph->graph(2)->setData(x2, y2);
    this->graph->graph(2)->setName("Fitted model");
    this->graph->replot();

}

void MainWindow::on_deletebutton_clicked()
{
    deleterow.clear();

    QStandardItemModel *_model = static_cast <QStandardItemModel*>(this->residualtable->model());

    // Get the changed checkbox.
    for (int i = 0; i < residualRow - 1; i++)
    {
        QStandardItem *Item = _model->item(i, 0);
        if (Item->checkState() == Qt::Checked)
        {
           deleterow.push_back(i);
           Item->setCheckState(Qt::Unchecked);
        }
    }
    if (deleterow.size() == 0)
    {
        QMessageBox::warning(this,tr("Warning"),("Please select at least one row."), QMessageBox::Yes);
    }
    else
    {
        deletetimes ++;
        restorebutton->setEnabled(true);
        this->textBrowser->clear();
    }

    // For simple regression
    if (deleterow.size() != 0 && methodtype == 1)
    {
        LSregression *newregression = this->stack_ls.push(deleterow);

        // Change summary
        std::vector<std::vector<std::string>> newsummary;
        std::vector<std::string> newtext;
        newregression->printSummary(newsummary, newtext);
        putsummary(newsummary, newtext);

        // Change table
        std::vector<std::vector<std::string> > analysis;
        newregression->ResidualAnalysis(iscookmeasure, analysis);
        putResidualsummary(analysis);

        // Draw graph
        // Draw scatter
        this->graph->clearGraphs();
        arma::mat X = newregression->getX();
        arma::mat Y = newregression->getY();
        plotScatter(X, Y, *newregression);
        // Draw line
        arma::mat linedata = newregression->getbetaHat();
        double data0 = linedata(0);
        double data1 = linedata(1);
        plotRegressionLine(data0, data1);
    }

    // For multiple regression
    if (deleterow.size() != 0 && methodtype == 2)
    {
        LSregression *newmulti = this->stack_multi.push(deleterow);

        // Change summary
        std::vector<std::vector<std::string>> newmultisummary;
        std::vector<std::string> newmultitext;
        newmulti->printSummary(newmultisummary, newmultitext);
        putsummary(newmultisummary, newmultitext);

        // Change table
        std::vector<std::vector<std::string> > analysis;
        newmulti->ResidualAnalysis(iscookmeasure, analysis);
        putResidualsummary(analysis);

        this->tabWidget->setCurrentIndex(0);

    }

    // For robust regression
    if (deleterow.size() != 0 && methodtype == 3)
    {
        // Resolve
        LSregression* ls = this->stack_rob.push(deleterow);
        ls->setSignificance(significance_num);
        ls->solve();
        this->pri_robregression.setLSR(*ls);
        this->pri_robregression.setT(t_num);
        this->pri_robregression.solve();

        // Change summary

        std::vector<std::vector<std::string>> newrobsummary;
        std::vector<std::string> newrobtext;
        this->pri_robregression.printSummary(newrobsummary, newrobtext);
        putsummary(newrobsummary, newrobtext);

        // Change table
        std::vector<std::vector<std::string> > analysis;
        this->pri_robregression.ResidualAnalysis(iscookmeasure, analysis);
        putResidualsummary(analysis);

        if (dataX.size() == 1)
        {
            // Draw graph
            // Draw scatter
            this->graph->clearGraphs();
            arma::mat X = this->pri_robregression.getX();
            arma::mat Y = this->pri_robregression.getY();
            plotScatter(X, Y, pri_robregression);
            // Draw line
            arma::mat linedata = this->pri_robregression.getbetaHat();
            double data0 = linedata(0);
            double data1 = linedata(1);
            plotRegressionLine(data0, data1);
        }
    }
}

void MainWindow::on_restorebutton_clicked()
{
    deletetimes --;
    deleterow.clear();
    if (deletetimes == 0)
    {
        restorebutton->setEnabled(false);
    }

    // For simple least square regression
    if (methodtype == 1)
    {
        this->stack_ls.pop();
        LSregression *newls = this->stack_ls.peek();

        // Change summary
        std::vector<std::vector<std::string>> newlssummary;
        std::vector<std::string> newlstext;
        newls->printSummary(newlssummary, newlstext);
        putsummary(newlssummary, newlstext);

        // Change table
        std::vector<std::vector<std::string> > newlsanalysis;
        newls->ResidualAnalysis(iscookmeasure, newlsanalysis);
        putResidualsummary(newlsanalysis);

        // Draw graph
        // Draw scatter
        this->graph->clearGraphs();
        arma::mat X = newls->getX();
        arma::mat Y = newls->getY();
        plotScatter(X, Y, *newls);
        // Draw line
        arma::mat linedata = newls->getbetaHat();
        double data0 = linedata(0);
        double data1 = linedata(1);
        plotRegressionLine(data0, data1);

    }

    // For multiple least square regression
    if (methodtype == 2)
    {
        this->stack_multi.pop();
        LSregression *newmulti = this->stack_multi.peek();


        // Change summary
        std::vector<std::vector<std::string>> newmultisummary;
        std::vector<std::string> newmultitext;
        newmulti->printSummary(newmultisummary, newmultitext);
        putsummary(newmultisummary, newmultitext);

        // Change table
        std::vector<std::vector<std::string> > analysis;
        newmulti->ResidualAnalysis(iscookmeasure, analysis);
        putResidualsummary(analysis);

        this->tabWidget->setCurrentIndex(0);

    }

    // For robust regression
    if (methodtype == 3)
    {
        this->stack_rob.pop();
        LSregression *newrob = this->stack_rob.peek();

        // Resolve
        newrob->setSignificance(significance_num);
        newrob->solve();
        this->pri_robregression.setLSR(*newrob);
        this->pri_robregression.setT(t_num);
        this->pri_robregression.solve();

        // Change summary

        std::vector<std::vector<std::string>> newrobsummary;
        std::vector<std::string> newrobtext;
        this->pri_robregression.printSummary(newrobsummary, newrobtext);
        putsummary(newrobsummary, newrobtext);

        // Change table
        std::vector<std::vector<std::string> > analysis;
        this->pri_robregression.ResidualAnalysis(iscookmeasure, analysis);
        putResidualsummary(analysis);

        if (dataX.size() == 1)
        {
            // Draw graph
            // Draw scatter
            this->graph->clearGraphs();
            arma::mat X = this->pri_robregression.getX();
            arma::mat Y = this->pri_robregression.getY();
            plotScatter(X, Y, pri_robregression);
            // Draw line
            arma::mat linedata = this->pri_robregression.getbetaHat();
            double data0 = linedata(0);
            double data1 = linedata(1);
            plotRegressionLine(data0, data1);
        }

    }
}
