#pragma once

#include <QByteArray>
#include <QPoint>

typedef struct TileOwn {
	QPoint leftBottomCorner;
	QByteArray data;
} TileOwn;

