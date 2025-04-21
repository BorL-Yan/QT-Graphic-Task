#include <QCoreApplication>
#include <QDebug>
#include "HgtLoader.h"
#ifdef GDAL_AVAILABLE
	#include "Loaders/HgtLoaderGdem.h"
#endif
#include "Loaders/HgtLoaderSrtm.h"
#include "Geo/GeoConstants.h"

HgtLoader* HgtLoader::m_instance;

HgtLoader* HgtLoader::instance() {
    return m_instance;
}

void HgtLoader::initHgtLoader(QString hgtArchiveDir, HgtType hgtType) {
    m_instance = new HgtLoader(hgtArchiveDir, hgtType);
}

HgtLoader::HgtLoader(QString hgtArchiveDir, HgtType hgtType)
    : QObject(nullptr)
{
	qRegisterMetaType<TileOwn>("TileOwn");

    m_settings.hgtArchiveDir = hgtArchiveDir;

#ifdef GDAL_AVAILABLE
    m_hgtType = hgtType;
#endif

	initHgtType(m_hgtType);
}

HgtLoader::~HgtLoader()
{
	m_hgtLoaderCore->deleteLater();
    m_hgtLoaderCore = nullptr;
}

void HgtLoader::setHgtType(int type)
{
	auto hgtType = static_cast<HgtType>(type);
	setHgtType(hgtType);
}

void HgtLoader::setHgtType(HgtType type)
{
	initHgtType(type);
}

void HgtLoader::initHgtType(HgtType type)
{
	if (m_hgtType == type && m_hgtLoaderCore) {
		return;
	}

	m_hgtType = type;
	if (m_hgtLoaderCore) {
		m_hgtLoaderCore->deleteLater();
		m_hgtLoaderCore = nullptr;
	}

    m_settings.hgtCachePath = hgtPath(m_settings.hgtArchiveDir, m_hgtType);

    if (m_hgtType == HgtType::SRTM || m_hgtType == HgtType::Unknown) {
        m_hgtLoaderCore = new HgtLoaderSrtm(&m_settings, this);
		m_hgtType = HgtType::SRTM;
	}
	else if(m_hgtType == HgtType::GDEM) {
#ifdef GDAL_AVAILABLE
        m_hgtLoaderCore = new HgtLoaderGdem(&m_settings, this);
		m_hgtType = HgtType::GDEM;
#else
		initHgtType(HgtType::SRTM);
		return;
#endif
	}
}

void HgtLoader::setCacheDirectory(const QString& hgtCachePath)
{
    m_settings.hgtCachePath = QDir(hgtCachePath).absolutePath();
}

void HgtLoader::setServerAddress(const QString& serverAddress)
{
    m_settings.serverAddress = serverAddress;
}

void HgtLoader::setOnlyFromCache(const bool onlyFromCache)
{
	m_settings.onlyFromCache = onlyFromCache;
}

bool HgtLoader::isOnlyFromCache() const
{
    return m_settings.onlyFromCache;
}

bool HgtLoader::getElevationFromTile(qint16& elevation, const double lon, const double lat, const QByteArray data) const
{
	return m_hgtLoaderCore->getElevationFromTile(elevation, lon, lat, data);
}

bool HgtLoader::getHgtFileOffset(qint64& offset, const double lon, const double lat) const
{
	return m_hgtLoaderCore->getHgtFileOffset(offset, lon, lat);
}

bool HgtLoader::gdalAvailable()
{
#ifdef GDAL_AVAILABLE
    return true;
#endif
    return false;
}

bool HgtLoader::getElevation(qint16& elevation, QPointF lonlat, const bool saveTileInsideModule)
{
    return getElevation(elevation, lonlat.x(), lonlat.y(), saveTileInsideModule);
}

bool HgtLoader::getElevation(qint16& elevation, const double lon, const double lat, const bool saveTileInsideModule)
{
	return m_hgtLoaderCore->getElevation(elevation, lon,lat, saveTileInsideModule);
}

QVector<QPointF> HgtLoader::getLeftBottomLocalHgt(const QString& dirPath)
{
	return m_hgtLoaderCore->getLeftBottomLocalHgt(dirPath);
}

QList<TileOwn> HgtLoader::getHgtTilesByRect(const double minLon, const double maxLon, const double minLat, const double maxLat, const bool saveTilesInsideModule)
{
	return m_hgtLoaderCore->getHgtTilesByRect(minLon, maxLon, minLat, maxLat, saveTilesInsideModule);
}

QList<TileOwn> HgtLoader::getHgtTilesByPolygon(const QList<QPointF> &nodes, const bool saveTilesInsideModule)
{
	return m_hgtLoaderCore->getHgtTilesByPolygon(nodes, saveTilesInsideModule);
}

bool HgtLoader::doesExistHgtByRect(const double minLon, const double maxLon, const double minLat, const double maxLat, QList<QString>& requiredFiles, const bool exitOnFirstFail)
{
	if (m_settings.hgtCachePath.isEmpty()) {
        qDebug() <<"HgtLoader.isExistsHgt. Cache hgt directory is not exists.";
		return false;
	}

	if (!isCorrectPoint(minLon, minLat)) {
        qDebug() <<"HgtLoader.isExistsHgt. Min lon,lat incorrect.";
		return false;
	}
	if (!isCorrectPoint(maxLon, maxLat)) {
        qDebug() <<"HgtLoader.isExistsHgt. Max lon,lat incorrect.";
		return false;
	}

	int lon1 = floor(minLon);
	int lat1 = floor(minLat);

	int lon2 = floor(maxLon);
	int lat2 = floor(maxLat);

	int lonmin = std::min(lon1,lon2);
	int lonmax = std::max(lon1,lon2);
	int latmin = std::min(lat1,lat2);
	int latmax = std::max(lat1,lat2);

	QString hgtFileName;
	QString srcHgtFileName;
	bool retVal = true;
	bool cycle = false;

	for(int lon=lonmin; lon<=lonmax; ++lon) {
		for(int lat=latmin; lat<=latmax; ++lat) {
			cycle = true;
			hgtFileName = m_hgtLoaderCore->getHgtHalfPathFileName((double)lon, (double)lat);
			if (hgtFileName.isEmpty()) {
                qDebug() <<QString("HgtDetect.isExistsHgt. hgtFileName.isEmpty lon=%1, lat=%2").arg(lon).arg(lat);
				return false;
			}
			srcHgtFileName = QDir::toNativeSeparators(QDir(m_settings.hgtCachePath).absolutePath() + QDir::separator() + hgtFileName);
			requiredFiles.append(srcHgtFileName);
			if (!QFile::exists(srcHgtFileName)) {
				retVal = false;
				if (exitOnFirstFail) {
					return false;
				}
			}
		}
	}
	if (!cycle) {
		retVal = false;
	}
	return retVal;
}

QString HgtLoader::getHgtFilePathAndNameFromCoordinates(const QPointF& geoPos)
{
	return m_hgtLoaderCore->getHgtFilePathAndNameFromCoordinates(geoPos);
}

bool HgtLoader::doesExistHgtByRect(const double minLon, const double maxLon, const double minLat, const double maxLat, const bool exitOnFirstFail)
{
	QList<QString> list;
	bool retV = doesExistHgtByRect(minLon, maxLon, minLat, maxLat, list, exitOnFirstFail);
	return retV;
}

bool HgtLoader::doesExistHgtByPolygon(const QList<QPointF>& nodes, QList<QString>& requiredFiles)
{
	bool retVal = false;

	if (nodes.count() == 0) return retVal;

	double minLon = std::numeric_limits<double>::max();
	double minLat = std::numeric_limits<double>::max();
	double maxLon = -std::numeric_limits<double>::max();
	double maxLat = -std::numeric_limits<double>::max();

	for( int i = 0; i < nodes.count(); i++ ) {
		auto item = nodes.at(i);
		if (item.x() < minLon) minLon = item.x();
		if (item.x() > maxLon) maxLon = item.x();
		if (item.y() < minLat) minLat = item.y();
		if (item.y() > maxLat) maxLat = item.y();
	}

	if (!Geo::Constants::isCorrectCoord(minLon)) return retVal;
	if (!Geo::Constants::isCorrectCoord(minLat)) return retVal;

	retVal = doesExistHgtByRect(minLon, maxLon, minLat, maxLat, requiredFiles);

	return retVal;
}

bool HgtLoader::doesExistHgtByPolygon(const QList<QPointF> &nodes)
{
	QList<QString> list;
	bool retV = doesExistHgtByPolygon(nodes, list);
	return retV;
}

bool HgtLoader::isCorrectPoint(const double lon, const double lat) const
{
	if ((lon < Geo::Constants::GEO_MIN_LON) || (lon > Geo::Constants::GEO_MAX_LON)) {
		return false;
	}

	if ((lat < Geo::Constants::GEO_MIN_LAT) || (lat > Geo::Constants::GEO_MAX_LAT)) {
		return false;
	}

	return true;
}
