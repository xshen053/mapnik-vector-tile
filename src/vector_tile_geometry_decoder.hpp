#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_DECODER_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_DECODER_H__

// mapnik-vector-tile
#include "vector_tile_config.hpp"

//libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

//protozero
#include <protozero/pbf_reader.hpp>

//mapnik
#include <mapnik/box2d.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/util/is_clockwise.hpp>
#if defined(DEBUG)
#include <mapnik/debug.hpp>
#endif

//std
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace mapnik 
{ 
    
namespace vector_tile_impl 
{

// NOTE: this object is for one-time use.  Once you've progressed to the end
//       by calling next(), to re-iterate, you must construct a new object
template <typename T>
class Geometry
{
public:
    using value_type = T;
    explicit Geometry(vector_tile::Tile_Feature const& f,
                             value_type tile_x, value_type tile_y,
                             double scale_x, double scale_y);

    enum command : uint8_t {
        end = 0,
        move_to = 1,
        line_to = 2,
        close = 7
    };

    bool scaling_reversed_orientation() const
    {
        return (scale_x_ * scale_y_) < 0;
    }

    uint32_t get_length() const
    {
        return length;
    }

    command point_next(value_type & rx, value_type & ry);
    command line_next(value_type & rx, value_type & ry, bool skip_lineto_zero);
    command ring_next(value_type & rx, value_type & ry, bool skip_lineto_zero);
    
private:
    vector_tile::Tile_Feature const& f_;
    double scale_x_;
    double scale_y_;
    uint32_t k;
    uint32_t geoms_;
    value_type x, y;
    value_type ox, oy;
    uint32_t length;
    uint8_t cmd;
    #if defined(DEBUG)
public:
    bool already_had_error;
    #endif
};

// NOTE: this object is for one-time use.  Once you've progressed to the end
//       by calling next(), to re-iterate, you must construct a new object
template <typename T>
class GeometryPBF 
{
public:
    using value_type = T;
    using pbf_itr = std::pair<protozero::pbf_reader::const_uint32_iterator, protozero::pbf_reader::const_uint32_iterator >;
    
    explicit GeometryPBF(pbf_itr const& geo_iterator,
                         value_type tile_x, 
                         value_type tile_y,
                         double scale_x, 
                         double scale_y);

    enum command : uint8_t
    {
        end = 0,
        move_to = 1,
        line_to = 2,
        close = 7
    };


    bool scaling_reversed_orientation() const
    {
        return (scale_x_ * scale_y_) < 0;
    }

    uint32_t get_length() const
    {
        return length;
    }

    command point_next(value_type & rx, value_type & ry);
    command line_next(value_type & rx, value_type & ry, bool skip_lineto_zero);
    command ring_next(value_type & rx, value_type & ry, bool skip_lineto_zero);

private:
    std::pair< protozero::pbf_reader::const_uint32_iterator, protozero::pbf_reader::const_uint32_iterator > geo_iterator_;
    double scale_x_;
    double scale_y_;
    value_type x, y;
    value_type ox, oy;
    uint32_t length;
    uint8_t cmd;
    #if defined(DEBUG)
public:
    bool already_had_error;
    #endif
};

template <typename T>
MAPNIK_VECTOR_INLINE mapnik::geometry::geometry<typename T::value_type> decode_geometry(T & paths, 
                                                                   int32_t geom_type, 
                                                                   unsigned version,
                                                                   mapnik::box2d<double> const& bbox);

template <typename T>
MAPNIK_VECTOR_INLINE mapnik::geometry::geometry<typename T::value_type> decode_geometry(T & paths, int32_t geom_type, unsigned version);

} // end ns vector_tile_impl

} // end ns mapnik

#endif // __MAPNIK_VECTOR_TILE_GEOMETRY_DECODER_H__
