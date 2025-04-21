#pragma once

#include <QObject>
#include <QString>
#include <QDataStream>
#include <QList>
#include <QVector>
#include <QPointF>
#include <QFile>
#include <limits>
#include <QReadWriteLock>
#include "./Loaders/IHgtLoader.h"

class QThread;
class QRectF;

//-32768
const short ERROR_ELEVATION_SRTM_HGT = std::numeric_limits<short>::min();

class HgtLoader :  public QObject
{
	Q_OBJECT

public:
    HgtLoader(QString hgtArchiveDir = "", HgtType hgtType = HgtType::SRTM);
	~HgtLoader();

    static void initHgtLoader(QString hgtArchiveDir, HgtType hgtType);
    static HgtLoader* instance();
    HgtType getHgtType() const {return m_hgtType;}
	void setHgtType(int type);
	void setHgtType(HgtType type);

	void setCacheDirectory(const QString &hgtCachePath);
    void setServerAddress(const QString &serverAddress);

    void setOnlyFromCache(const bool onlyFromCache);
    bool isOnlyFromCache() const;

    /**
	 * @brief checks does the region exist in cache, fills requiredFiles List with required .hgt files. First you have to set Cache Directory
	 */
	bool doesExistHgtByPolygon(const QList<QPointF>& nodes, QList<QString>& requiredFiles);
	bool doesExistHgtByPolygon(const QList<QPointF>& nodes);

	bool doesExistHgtByRect(const double minLon, const double maxLon, const double minLat, const double maxLat, QList<QString>& requiredFiles, const bool exitOnFirstFail = false);
	bool doesExistHgtByRect(const double minLon, const double maxLon, const double minLat, const double maxLat, const bool exitOnFirstFail = false);

	/**
     * @brief looks for .hgt file in saved tile map, then looks for .hgt file in cache.
	 * file from server to cache.
	 * @param saveTileInsideModule: set 'true' if you want to save tile inside module
	 */
	bool getElevation(qint16& elevation, const double lon, const double lat, const bool saveTileInsideModule = true);
	bool getElevation(qint16& elevation, QPointF lonlat, const bool saveTileInsideModule = true);

	QString getHgtFilePathAndNameFromCoordinates(const QPointF& geoPos);

	QVector<QPointF> getLeftBottomLocalHgt(const QString& dirPath);

	/**
	 * @brief looks for .hgt files in saved tile map, then looks for .hgt files in cache. Then tries to download
	 * files from server to cache.
	 * @param saveTileInsideModule: set 'true' if you want to save tiles inside module
	 * @return QList of tiles that were found in RAM or cache.
	 */
	QList<TileOwn> getHgtTilesByPolygon(const QList<QPointF>& nodes, const bool saveTilesInsideModule);
	QList<TileOwn> getHgtTilesByRect(const double minLon, const double maxLon,
											  const double minLat, const double maxLat, const bool saveTilesInsideModule);

	bool getElevationFromTile(qint16& elevation, const double lon, const double lat, const QByteArray data) const;

	bool getHgtFileOffset(qint64& offset, const double lon, const double lat) const;

	bool gdalAvailable();

private:
	bool isCorrectPoint(const double lon, const double lat) const;
	int getLonIndex(const double lon) const;
	int getLatIndex(const double lat) const;
	QString getLonCellName(const double lon) const;
	QString getLatCellName(const double lat) const;

	// N59E029.hgt
	// N60E030.hgt
	QString getHgtName(const double lon, const double lat) const;

	QPointF getLeftBottomNode(const QString& hgtName);

	void initHgtType(HgtType type);    

private:
    static HgtLoader* m_instance;
    const QString separator = "/";
	HgtType m_hgtType = HgtType::SRTM;
	IHgtLoader* m_hgtLoaderCore = nullptr;
	HgtSettings m_settings;
};
