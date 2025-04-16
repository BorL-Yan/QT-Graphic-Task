#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QChartView>
#include <QVector>
#include "point.h"
#include <QtCharts>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:

    QVector<QChartView*> chartViews;

    //Data
    QList<Point> inputList;
    QVector<QVector<qint16>> hgtData;
    double latStart = 40.0;
    double lonStart = 42.0;
    int gridSize = 1201;


    //Metods
    bool readHGT(const QString &filePath);
    double getHGTHeight(double x, double y);
    void createPathPoints(QVector<QPointF> &pathPoints, QVector<double> &distances, const QList<Point> &sortedList);

    void updateCharts();
    void loadHGT();

};
#endif // MAINWINDOW_H
