#pragma once

#include <QObject>
#include <QString>
#include <QPointF>
#include <QThread>
#include <QDir>
#include <QCoreApplication>

#include "../HgtSettings.h"

enum class HgtType {
	Unknown,
	SRTM,
	GDEM
};

inline QString hgtPath(QString hgtArchiveDir, HgtType hgtType) {
    if(QDir::isRelativePath(hgtArchiveDir)) {
        hgtArchiveDir = QCoreApplication::applicationDirPath() + QDir::separator() + hgtArchiveDir;
    }
    return QDir(hgtArchiveDir).absolutePath() + QDir::separator() + (hgtType == HgtType::GDEM ? "gdem" : "hgt");
}

class IHgtLoader : public QObject
{
	Q_OBJECT

public:
	IHgtLoader(QObject* parent = nullptr) : QObject(parent) {}
	virtual ~IHgtLoader() = default;

	/**
	 * @brief looks for .hgt file in saved tile map, then looks for .hgt file in cache. Then tries to download
	 * file from server to cache.
	 * @param saveTileInsideModule: set 'true' if you want to save tile inside module
	 */
	virtual bool getElevation(qint16& elevation, const double lon, const double lat, const bool saveTileInsideModule) = 0;

    virtual QString getHgtFilePathAndNameFromCoordinates(const QPointF& geoPos) = 0;

    virtual QVector<QPointF> getLeftBottomLocalHgt(const QString& dirPath) = 0;

	/**
	 * @brief looks for .hgt files in saved tile map, then looks for .hgt files in cache. Then tries to download
	 * files from server to cache.
	 * @param saveTileInsideModule: set 'true' if you want to save tiles inside module
	 * @return QList of tiles that were found in RAM or cache.
	 */
	virtual QList<TileOwn> getHgtTilesByPolygon(const QList<QPointF>& nodes, const bool saveTilesInsideModule) = 0;
	virtual QList<TileOwn> getHgtTilesByRect(const double minLon, const double maxLon,
											  const double minLat, const double maxLat, const bool saveTilesInsideModule) = 0;

    virtual bool getElevationFromTile(qint16& elevation, const double lon, const double lat, const QByteArray data) = 0;
	// O/35/N59E029.hgt
	// P/36/N60E030.hgt
	virtual QString getHgtHalfPathFileName(const double lon, const double lat) const = 0;

	virtual bool getHgtFileOffset(qint64& offset, const double lon, const double lat) const = 0;

};
