#include "mainwindow.h"

#include <QHBoxLayout>
#include <QFile>
#include <QDataStream>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>
#include <QGeoCoordinate>
#include <algorithm>

using namespace QtCharts;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *centreWidget = new QWidget(this);
    setCentralWidget(centreWidget);
    QHBoxLayout *mainLayout = new QHBoxLayout(centreWidget);

    resize(800,600);

    QChartView *heightView = new QChartView(this); // Для высот.
    heightView->setRenderHint(QPainter::Antialiasing);

    // Добавление графиков в chartViews и макет.
    chartViews.append(heightView);
    mainLayout->addWidget(heightView);

    inputList.append(Point(42.1, 40.1, 1900.0));
    inputList.append(Point(42.2, 40.2, 1800.0));
    inputList.append(Point(42.3, 40.1, 2100.0));
    inputList.append(Point(42.9, 40.1, 1980.0));

    loadHGT();

    updateCharts();
}

MainWindow::~MainWindow()
{
    for (QChartView *view : chartViews)
    {
        delete view;
    }
}

void MainWindow::loadHGT()
{
    QString filePath = "E:/Qt Projects/QT-Graphic-Task/GraphicCreat/K38/N40E042.hgt";

    if(!readHGT(filePath)){
        qWarning() << "Error : HGT Loading file" << filePath;
    }

}

bool MainWindow::readHGT(const QString &filePath)
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly)){
        return false;
    }

    hgtData.clear();
    hgtData.resize(gridSize);
    for(int i = 0; i < gridSize; ++i){
        hgtData[i].resize(gridSize);
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::BigEndian);;
    for (int i = 0; i < gridSize; ++i) {
        for (int j = 0; j < gridSize; ++j) {
            qint16 height;
            stream >> height;
            hgtData[i][j] = height;
        }
    }

    file.close();
    return true;
}

double MainWindow::getHGTHeight(double x, double y)
{
    if(hgtData.isEmpty()){
        return 0.0;
    }

    double step = 1.0 / (gridSize - 1);
    double iFrac = (1.0 - (y - latStart)) / step;
    double jFrac = (x - lonStart) / step;
    int i = qBound(0, static_cast<int>(iFrac), gridSize - 1);
    int j = qBound(0, static_cast<int>(jFrac), gridSize - 1);

    return hgtData[i][j];
}


void MainWindow::createPathPoints(QVector<QPointF> &pathPoints, QVector<double> &distances, const QList<Point> &sortedList)
{
    pathPoints.clear();
    distances.clear();

    if(inputList.size() < 2){
        return;
    }

    double totalDistance = 0.0;
    QGeoCoordinate prevCoord(sortedList[0].y, sortedList[0].x);

    pathPoints.append(QPointF(0.0, sortedList[0].h));
    distances.append(0.0);

    for (int i = 1; i < sortedList.size(); ++i) {
        QGeoCoordinate currCoord(sortedList[i].y, sortedList[i].x);
        double segmentLenght = prevCoord.distanceTo(currCoord);

        for (int j = 0; j <= 100; ++j) {
            double t = j / 100.0;
            double x = sortedList[i-1].x + t * (sortedList[i].x - sortedList[i-1].x);
            double y = sortedList[i-1].y + t * (sortedList[i].y - sortedList[i-1].y);
            double h = getHGTHeight(x,y);

            double dist = totalDistance + t * segmentLenght;
            pathPoints.append(QPointF(dist, h));
            distances.append(dist);
        }

        totalDistance += segmentLenght;
        pathPoints.append(QPointF(totalDistance, sortedList[i].h));
        distances.append(totalDistance);

        prevCoord = currCoord;
    }
}

void MainWindow::updateCharts()
{
    QList<Point> sortedList = inputList;
    std::sort(sortedList.begin(), sortedList.end(), [](const Point &a, const Point &b){
        return (a.x == b.x) ? a.y < b.y: a.x < b.x;
    });


    // Creath Graphic
    QChart *heightChart = new QChart();

    QLineSeries *heightSeries = new QLineSeries();
    QLineSeries *pointLineSeries = new QLineSeries();
    QScatterSeries *pointSeries = new QScatterSeries();
    heightChart->setTitle("Graphic");
    pointSeries->setMarkerSize(12.0);
    pointSeries->setColor(Qt::blue);
    pointLineSeries->setColor(Qt::blue);


    // Переменные для осей.
    double minH = sortedList.isEmpty() ? 0.0 : sortedList[0].h;
    double maxH = minH;
    double maxDistance = 0.0;


    // Если есть минимум 2 точки, строим полный профиль.
    if (sortedList.size() >= 2) {
        QVector<QPointF> pathPoints;
        QVector<double> distances;
        createPathPoints(pathPoints, distances, sortedList);



        double totalDistance = 0.0;
        QGeoCoordinate prevCoord(sortedList[0].y, sortedList[0].x);
        pointSeries->append(0.0, sortedList[0].h);
        pointLineSeries->append(0.0, sortedList[0].h);
        if (!hgtData.isEmpty()) {
            for (const QPointF &pt : pathPoints) {
                heightSeries->append(pt);
                minH = qMin(minH, pt.y());
                maxH = qMax(maxH, pt.y());
            }
            maxDistance = distances.last();

            for (int i = 1; i < sortedList.size(); ++i) {
                QGeoCoordinate currCoord(sortedList[i].y, sortedList[i].x);
                totalDistance += prevCoord.distanceTo(currCoord);
                pointSeries->append(totalDistance, sortedList[i].h);
                pointLineSeries->append(totalDistance, sortedList[i].h);
                minH = qMin(minH, sortedList[i].h);
                maxH = qMax(maxH, sortedList[i].h);
                prevCoord = currCoord;
            }
        }else{
            heightSeries->append(0.0, sortedList[0].h);
            for (int i = 1; i < sortedList.size(); ++i) {
                QGeoCoordinate currCoord(sortedList[i].y, sortedList[i].x);
                totalDistance += prevCoord.distanceTo(currCoord);
                heightSeries->append(totalDistance, sortedList[i].h);
                prevCoord = currCoord;
            }
            maxDistance = totalDistance;
        }

    } else if (!sortedList.isEmpty()) {
        // Если только одна точка, показываем её.
        pointSeries->append(0.0, sortedList[0].h);
        maxDistance = 1.0; // Минимальный диапазон.
    }



    // Настройка осей графика высот.
    QValueAxis *heightAxisX = new QValueAxis();
    heightAxisX->setTitleText("Расстояние (м)");
    heightAxisX->setRange(-100, maxDistance > 0.0 ? maxDistance + 100: 110.0);
    QValueAxis *heightAxisY = new QValueAxis();
    heightAxisY->setTitleText("Высота (м)");
    heightAxisY->setRange(minH - 100, maxH + 100);

    heightChart->addSeries(heightSeries);
    heightChart->addSeries(pointSeries);
    heightChart->addSeries(pointLineSeries);

    // Привязка осей и серий.
    heightChart->addAxis(heightAxisX, Qt::AlignBottom);
    heightChart->addAxis(heightAxisY, Qt::AlignLeft);

    heightSeries->attachAxis(heightAxisX);
    heightSeries->attachAxis(heightAxisY);

    pointSeries->attachAxis(heightAxisX);
    pointSeries->attachAxis(heightAxisY);

    pointLineSeries->attachAxis(heightAxisY);
    pointLineSeries->attachAxis(heightAxisX);

    // Установка графика в chartViews[0].
    chartViews[0]->setChart(heightChart);
}






















