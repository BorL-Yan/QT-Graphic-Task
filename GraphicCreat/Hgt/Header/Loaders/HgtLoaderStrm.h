

#include <QObject>
#include <QString>
#include <QDataStream>
#include <QList>
#include <QVector>
#include <QPointF>
#include <QFile>
#include <limits>
#include <QMap>
#include <QMutex>
#include "IHgtLoader.h"
#include "../HgtSettings.h"

class QThread;
class QRectF;

struct SrtmCache {
    SrtmCache() {}
    ~SrtmCache() {
        delete hgtFile; //this will unmap and close the file
    }
    QFile*  hgtFile = nullptr;
    quint8* data = nullptr;
    qint64 fileSize = 0;
};

class HgtLoaderSrtm : public IHgtLoader
{
public:
    HgtLoaderSrtm(HgtSettings* settings, QObject* parent = 0);
	~HgtLoaderSrtm();

	/**
	 * @brief looks for .hgt file in saved tile map, then looks for .hgt file in cache. Then tries to download
	 * file from server to cache.
	 * @param saveTileInsideModule: set 'true' if you want to save tile inside module
	 */
	bool getElevation(qint16& elevation, const double lon, const double lat, const bool saveTileInsideModule);

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

	/**
	 * @return QList of tiles stored in this module
	 */
	//QList<TileOwn> getSavedMap();

	bool getElevationFromTile(qint16& elevation, const double lon, const double lat, const QByteArray data);
	// O/35/N59E029.hgt
	// P/36/N60E030.hgt
	QString getHgtHalfPathFileName(const double lon, const double lat) const;

    bool getHgtFileOffset(qint64& offset, const double lon, const double lat) const;

private:
	bool isCorrectPoint(const double lon, const double lat) const;
	int getLonIndex(const double lon) const;
	int getLatIndex(const double lat) const;
	QString getLonCellName(const double lon) const;
	QString getLatCellName(const double lat) const;
    HgtSettings* m_settings = nullptr;
    QMutex  m_cacheLock;
    QList<quint32> m_CashKey;
    QMap<quint32, SrtmCache*> m_Cash;

    bool getHgt(const double lon, const double lat, QByteArray *dat, qint16* elevation = nullptr);

	// N59E029.hgt
	// N60E030.hgt
	QString getHgtName(const double lon, const double lat) const;

	QPointF getLeftBottomNode(const QString& hgtName);

private:
	const QString hgtExtension = ".hgt";

};

