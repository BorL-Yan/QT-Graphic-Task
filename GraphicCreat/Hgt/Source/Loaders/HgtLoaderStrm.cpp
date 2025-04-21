#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QStringList>
#include <QtEndian>
#include <QtMath>
#include <QRectF>
#include <QCoreApplication>
#include <QMetaType>
#include <QMutexLocker>
#include <QDebug>
#include "HgtLoaderSrtm.h"
#include "../Geo/GeoConstants.h"

//degree per pixel
const double DEG_PER_PIX_SRTM_HGT = 1.0/1200.0;
//degree per half pixel
const double DEG_PER_HALF_PIX_SRTM_HGT = 1.0/2400.0;
//number of pixels in width and height
const int SRTM_SIDE_SIZE_SRTM_HGT = 1201;
//2 bytes in hgt file
const int SIZE_ELEVATION_SRTM_HGT = 2;

const int ERROR_LONLAT_INDEX_SRTM_HGT = -1;

inline quint32 makeKeyLatLon(int lat, int lon) {
	return lat << 16 | lon;
}

HgtLoaderSrtm::HgtLoaderSrtm(HgtSettings *settings, QObject* parent) :
    m_settings(settings),
    IHgtLoader(parent)
{
}

HgtLoaderSrtm::~HgtLoaderSrtm()
{    
}

bool HgtLoaderSrtm::getElevationFromTile(qint16& elevation, const double lon, const double lat, const QByteArray data)
{
	qint64 eleOffset = 0;
	if (!getHgtFileOffset(eleOffset, lon, lat)) {
		return false;
	}

	if (eleOffset+sizeof(elevation) > data.size()) {
		return false;
	}

	memcpy(&elevation, data.data()+eleOffset, sizeof(elevation));
	elevation = qFromBigEndian(elevation);

	return true;
}

bool HgtLoaderSrtm::getHgtFileOffset(qint64& offset, const double lon, const double lat) const
{
	int lonName = floor(lon);
	int latName = floor(lat);

	double lonCol = (lon - ((double)lonName) - DEG_PER_HALF_PIX_SRTM_HGT)/DEG_PER_PIX_SRTM_HGT;
	double latRow = (((double)latName) + 1.0 + DEG_PER_HALF_PIX_SRTM_HGT - lat)/DEG_PER_PIX_SRTM_HGT;
	int col = floor(lonCol);
	int row = floor(latRow);

	if ((col < 0) || (col >= SRTM_SIDE_SIZE_SRTM_HGT)) {
		return false;
	}

	if ((row < 0) || (row >= SRTM_SIDE_SIZE_SRTM_HGT)) {
		return false;
	}

	offset = SIZE_ELEVATION_SRTM_HGT*(col + row*SRTM_SIDE_SIZE_SRTM_HGT);

	return true;
}

bool HgtLoaderSrtm::getElevation(qint16& elevation, const double lon, const double lat, const bool saveTileInsideModule)
{
    Q_UNUSED(saveTileInsideModule)
    return getHgt(lon, lat, nullptr, &elevation);
}

QVector<QPointF> HgtLoaderSrtm::getLeftBottomLocalHgt(const QString& dirPath)
{
	QVector<QPointF> retVal;

	if (dirPath.isEmpty()) return retVal;
	QDir dir(dirPath);
	if (!dir.exists()) return retVal;

	dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
	dir.setNameFilters( QStringList() << "*.hgt");

	QDirIterator it(dir, QDirIterator::Subdirectories);
	while(it.hasNext()) {
		QFileInfo fi(it.next());
		QPointF node = getLeftBottomNode(fi.fileName());
		if (!Geo::Constants::isCorrectCoord(node.x())) continue;
		retVal.append(node);
	}

	return retVal;
}

//N59E030.hgt
QPointF HgtLoaderSrtm::getLeftBottomNode(const QString& hgtName)
{
	QPointF retVal(Geo::Constants::INVALID_GEO_POS);

	int latPos = hgtName.indexOf("N");
	int latSign = 1;
	if (latPos < 0) {
		latPos = hgtName.indexOf("S");
		latSign = -1;
	}
	if (latPos < 0) return retVal;

	int lonPos = hgtName.indexOf("E",latPos+1);
	int lonSign = 1;
	if (lonPos < 0) {
		lonPos = hgtName.indexOf("W",latPos+1);
		lonSign = -1;
	}
	if (lonPos < 0) return retVal;

	int dotPos = hgtName.lastIndexOf(".");
	if (dotPos < 0) return retVal;

	latPos++;
	QString lat_str = hgtName.mid(latPos, lonPos-latPos);

	lonPos++;
	QString lon_str = hgtName.mid(lonPos, dotPos-lonPos);

	bool okNumber;
	int lat = lat_str.toInt(&okNumber);
	if (!okNumber) return retVal;
	int lon = lon_str.toInt(&okNumber);
	if (!okNumber) return retVal;

	retVal.setX(lon*lonSign);
	retVal.setY(lat*latSign);

	return retVal;
}

QList<TileOwn> HgtLoaderSrtm::getHgtTilesByRect(const double minLon, const double maxLon, const double minLat, const double maxLat, const bool saveTilesInsideModule)
{
    Q_UNUSED(saveTilesInsideModule)
	QList<TileOwn> map;
	if (!isCorrectPoint(minLon, minLat)) {
        qDebug() << "HgtLoaderSrtm.getHgtTilesByRect. Min lon,lat incorrect.";
		return map;
	}
	if (!isCorrectPoint(maxLon, maxLat)) {
        qDebug() << "HgtLoaderSrtm.getHgtTilesByRect. Max lon,lat incorrect.";
		return map;
	}

	int lon1 = floor(minLon);
	int lat1 = floor(minLat);

	int lon2 = floor(maxLon);
	int lat2 = floor(maxLat);

	int lonmin = std::min(lon1,lon2);
	int lonmax = std::max(lon1,lon2);
	int latmin = std::min(lat1,lat2);
	int latmax = std::max(lat1,lat2);

	for(int lon=lonmin; lon<=lonmax; ++lon) {
		for(int lat=latmin; lat<=latmax; ++lat) {
			QByteArray dat;
            if (!getHgt(lon,lat,&dat)) {
                qDebug() << "HgtLoaderSrtm.getHgtTilesByRect. Can't get hgt file from cache.";
				continue;
			}
			map.append({{lon,lat},dat});
		}
	}

	return map;
}

QList<TileOwn> HgtLoaderSrtm::getHgtTilesByPolygon(const QList<QPointF> &nodes, const bool saveTilesInsideModule)
{
	QList<TileOwn> map;

	if (nodes.count() == 0) return map;

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

	if (!Geo::Constants::isCorrectCoord(minLon)) return map;
	if (!Geo::Constants::isCorrectCoord(minLat)) return map;

	return getHgtTilesByRect(minLon, maxLon, minLat, maxLat, saveTilesInsideModule);
}

QString HgtLoaderSrtm::getHgtFilePathAndNameFromCoordinates(const QPointF& geoPos)
{
    int lon = floor(geoPos.x());
    int lat = floor(geoPos.y());
    QString hgtFileName = getHgtHalfPathFileName((double)lon, (double)lat);
    QString srcHgtFileName = QDir::toNativeSeparators(QDir(m_settings->hgtCachePath).absolutePath() + QDir::separator() + hgtFileName);
    return srcHgtFileName;
}

bool HgtLoaderSrtm::getHgt(const double lon, const double lat, QByteArray *dat, qint16* elevation)
{
    QMutexLocker locker(&m_cacheLock);

    int lonName = floor(lon);
	int latName = floor(lat);

    qint64 eleOffset = 0;
    if (elevation && !getHgtFileOffset(eleOffset, lon, lat)) {
//        STDLOG("Error on getHgtFileOffset: lonName=%lf latName=%lf\n", lon, lat);
        return false;
    } else {
//        STDLOG("OK on getHgtFileOffset: lonName=%lf latName=%lf\n", lon, lat);
    }

    quint32 latLon = makeKeyLatLon(latName, lonName);

    SrtmCache* srtm = nullptr;

    auto it = m_Cash.find(latLon);
    if (it != m_Cash.end()){
        srtm = it.value();
    } else {
        QString coordFileName = getHgtName(lon, lat);
        QString hgtFileName = QDir::toNativeSeparators(m_settings->hgtCachePath + QDir::separator() + coordFileName);
        std::string strFileName = hgtFileName.toStdString();
        //STDLOG("Openning: lon=%d lat=%d %s\n", lonName, latName, strFileName.c_str());

        srtm = new SrtmCache;
        srtm->hgtFile = new QFile(hgtFileName);
        if (!srtm->hgtFile->open(QIODevice::ReadOnly)) {
            qDebug() << QString("HgtLoaderSrtm.getHgt. Can't open file %1.").arg(hgtFileName);
        } else {
            srtm->fileSize = srtm->hgtFile->size();
            srtm->data = srtm->hgtFile->map(0, srtm->fileSize);
            if (srtm->data == nullptr) {
                qDebug() << QString("HgtLoaderSrtm.getHgt. Can't map file %1.").arg(hgtFileName);
            }
        }

        if (!srtm->data) {
            delete srtm;
            srtm = nullptr;
        }

        if (m_CashKey.size() == m_settings->maxNumOfTilesInRAM){
            auto key = m_CashKey.takeFirst();
            SrtmCache* val = m_Cash.take(key);
            delete val;
        }
        m_CashKey.append(latLon);
        m_Cash.insert(latLon, srtm);
    }

    if (!srtm) {
        return false;
    }

    if (elevation) {
        if (eleOffset + sizeof(*elevation) > std::size_t(srtm->fileSize)) {
            qDebug("Failed to get elevation: lonName=%d latName=%d\n", lonName, latName);
            return false;
        }
        memcpy(elevation, srtm->data+eleOffset, sizeof(*elevation));
        *elevation = qFromBigEndian(*elevation);
    }

    if (dat) {
        *dat = QByteArray::fromRawData(reinterpret_cast<char*>(srtm->data), srtm->fileSize);
    }

	return true;
}

bool HgtLoaderSrtm::isCorrectPoint(const double lon, const double lat) const
{

	if ((lon < Geo::Constants::GEO_MIN_LON) || (lon > Geo::Constants::GEO_MAX_LON)) {
		return false;
	}

	if ((lat < Geo::Constants::GEO_MIN_LAT) || (lat > Geo::Constants::GEO_MAX_LAT)) {
		return false;
	}

	return true;
}

int HgtLoaderSrtm::getLonIndex(const double lon) const
{
	int retVal = ERROR_LONLAT_INDEX_SRTM_HGT;
	if ((lon < Geo::Constants::GEO_MIN_LON) || (lon > Geo::Constants::GEO_MAX_LON)) {
		return retVal;
	}

	double longitude = 180.0 + lon;
	double zoneIndex = longitude/6.0;
	retVal = floor(zoneIndex);
	retVal++;
	if (retVal == 61) {
		retVal = 60;
	}

	return retVal;
}

int HgtLoaderSrtm::getLatIndex(const double lat) const
{
    int retVal = ERROR_LONLAT_INDEX_SRTM_HGT;
	if ((lat < Geo::Constants::GEO_MIN_LAT) || (lat > Geo::Constants::GEO_MAX_LAT)) {
		return retVal;
	}
	if (lat < -88.0) {
		return 0;
	}

	double latitude = 88.0 + lat;
	double zoneIndex = latitude/4.0;
	retVal = floor(zoneIndex);
	retVal++;

	return retVal;
}

QString HgtLoaderSrtm::getLonCellName(const double lon) const
{
    QString retVal = "";

	int lonIndex = getLonIndex(lon);
	if (lonIndex==ERROR_LONLAT_INDEX_SRTM_HGT) {
		return retVal;
	}

	retVal += QString("%1").arg(lonIndex, 2, 10, QLatin1Char('0'));

	return retVal;
}

QString HgtLoaderSrtm::getLatCellName(const double lat) const
{
	QString retVal = "";

	int latIndex = getLatIndex(lat);
	if (latIndex==ERROR_LONLAT_INDEX_SRTM_HGT) {
//		log_warning(QString("HgtDetect.getLatCellName. Incorrect lat index. lat=%1").arg(lat));
		return retVal;
	}

	if (latIndex >=23) {
		latIndex -= 22;
	}
	else {
		retVal = "S";
		latIndex = 23 - latIndex;
	}

	//65=A, 66=B, ... 85=U
	int code = 64 + abs(latIndex);
	char symbol = (char)code;
	retVal += symbol;

	return retVal;
}

QString HgtLoaderSrtm::getHgtName(const double lon, const double lat) const
{
    QString retVal = "";
	if (!isCorrectPoint(lon, lat)) {
        qDebug() << QString("HgtDetectZip.getHgtName. Incorrect lon=%1, lat=%2").arg(lon).arg(lat);
		return retVal;
	}

	int lonName = floor(lon);
	int latName = floor(lat);

	if (latName >= 0) {
		retVal += "N";
	}
	else {
		retVal += "S";
	}
	retVal += QString("%1").arg(abs(latName), 2, 10, QLatin1Char('0'));

	if (lonName >= 0) {
		retVal += "E";
	}
	else {
		retVal += "W";
	}
	retVal += QString("%1").arg(abs(lonName), 3, 10, QLatin1Char('0'));
	retVal += hgtExtension;

	return retVal;
}

QString HgtLoaderSrtm::getHgtHalfPathFileName(const double lon, const double lat) const
{
	QString retVal = "";

	QString lonCellName = getLonCellName(lon);
	if (lonCellName.isEmpty()) {
		return retVal;
	}

	QString latCellName = getLatCellName(lat);
	if (latCellName.isEmpty()) {
		return retVal;
	}

	QString hgtFileName = getHgtName(lon, lat);
	if (hgtFileName.isEmpty()) {
		return retVal;
	}

    retVal = latCellName + QDir::separator() + lonCellName + QDir::separator() + hgtFileName;

	return retVal;
}

