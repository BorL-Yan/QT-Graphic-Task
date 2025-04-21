#pragma once

#include <QString>
#include <QByteArray>
#include <QPoint>
#include "TileOwn.h"

typedef struct HgtSettings {
    QString hgtArchiveDir = "";
    QString hgtCachePath = "";
    QString serverAddress = "";
    int maxNumOfTilesInRAM = 10;
    bool onlyFromCache = true;
} HgtSettings;
