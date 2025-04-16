#ifndef POINT_H
#define POINT_H

struct Point{
    double x;
    double y;
    double h;

    Point(double x_ = 0.0, double y_ = 0.0, double h_ = 0.0)
        :x(x_), y(y_), h(h_)
    {}

};

#endif // POINT_H
