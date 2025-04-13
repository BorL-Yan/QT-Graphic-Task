#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    QLineSeries *series = new QLineSeries();
    for(int x = -10; x <= 10; x++){
        series->append(x, std::sin(x));
    }

    chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(" y = sin(x) ");
    chart->createDefaultAxes();
    chart->axes(Qt::Horizontal).first()->setTitleText("X");
    chart->axes(Qt::Vertical).first()->setTitleText("Y");

    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);


    setCentralWidget(chartView);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow :: on_updateButton_clicked(){
    chart->removeAllSeries();
    QLineSeries *series = new QLineSeries;
    for(int x = -10; x <= 10; x++){
        series->append(x, x * x);
    }
    chart->addSeries(series);
    chart->createDefaultAxes();
}
