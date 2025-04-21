#pragma once

#include <qglobal.h>
#include <QPointF>
#include <limits>
#include <cmath>

///////////////////////////////////////////////////////////////////////////////
namespace Geo {
///////////////////////////////////////////////////////////////////////////////

namespace Constants
{

const double EarthRad = 6371.0;
const double DEG2RAD1 = 1.74532925199e-02;

//Semi-major Axis WGS84
constexpr double EARTH_A_RADIUS = 6378137.0; //Google EPSG::3857, NOT Yandex(EPSG::3395)
//Flattening Factor WGS84
/*The WGS 84 datum surface is an oblate spheroid (ellipsoid)
 * with major (equatorial) radius a = 6378137 m at the equator and
 * flattening f = 1/298.257223563.
 * The polar semi-minor axis b then equals a × (1 − f) = 6356752.3142 m.
 * (https://en.wikipedia.org/wiki/World_Geodetic_System)
*/
constexpr double EARTH_FLATTENING_FACTOR = 1.0/298.257223563;
constexpr double EARTH_POLAR_AXIS_LENGTH = EARTH_A_RADIUS * (1.0 - EARTH_FLATTENING_FACTOR);

const double ERROR_COORDINATE = std::numeric_limits<double>::quiet_NaN();
inline bool isErrorCoordinate(const double value) {return std::isnan(value);}

const double GEO_ERROR_YAW = -180.0;

const uint ERROR_TIME = 0;

const double GEO_MAX_LON = 180.0;
const double GEO_MIN_LON = -180.0;

const double GEO_MAX_LAT = 90.0;
const double GEO_MIN_LAT = -90.0;
const QPointF INVALID_GEO_POS = QPointF(Constants::ERROR_COORDINATE, Constants::ERROR_COORDINATE);
inline bool isInvalidGeoPos(const QPointF value) {return std::isnan(value.x()) && std::isnan(value.y());}

const double ERROR_DISTANCE = -1.0;
const double ERROR_AZIMUTH = 361.0;

const double MAXIMUM_ROUTE_SEGMENT_DISTANCE = 20000.0; //meters
const double UTM_ZONE_SIZE_DEGREE = 6.0;

const int ERROR_PIX_COORD = -1;

const int ERROR_ZONE_INDEX = -1;

//-----vector map-----
const double PIX_PER_INCH = 96.0;
const double CM_PER_INCH = 2.54;
const int TILE_SIZE_PIX = 256;
const int MIN_TILE_IMAGE_SIZE_PIX = 16;

//sizeof(int)=4
//int min=-2147483648
//int max=2 147 483 647
//2^22 = 4 194 304 х 256 = 1 073 741 824
//2^23 = 8 388 608 х 256 = 2 147 483 648 (int max=2 147 483 647)
const int MIN_ZOOM_LEVEL = 0;
const int MAX_ZOOM_LEVEL = 22;
const int OSM_MAX_ZOOM_LEVEL = 19; //open street map 0..19
const int ERROR_ZOOM_LEVEL = -1;
const double ERROR_ZOOM = -1.0;
//-----vector map-----

bool isCorrectCoord(const double val);

bool isCorrectGeoCoord(const double lon, const double lat);

bool isCorrectGeoCoord(const QPointF& pos);

}

///////////////////////////////////////////////////////////////////////////////
} ///namespace Geo
///////////////////////////////////////////////////////////////////////////////
