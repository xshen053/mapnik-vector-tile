#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_DECODER_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_DECODER_H__

#include "vector_tile.pb.h"

namespace mapnik { namespace vector_tile_impl {

class Geometry {

public:
    inline explicit Geometry(vector_tile::Tile_Feature const& f,
                             double tile_x, double tile_y,
                             double scale_x, double scale_y);

    enum command : uint8_t {
        end = 0,
        move_to = 1,
        line_to = 2,
        close = 7
    };

    inline command next(double& rx, double& ry);

private:
    vector_tile::Tile_Feature const& f_;
    double scale_x_;
    double scale_y_;
    uint32_t k;
    uint32_t geoms_;
    uint8_t cmd;
    uint32_t length;
    double x, y;
    double ox, oy;
};


mapnik::geometry::geometry decode_geometry(vector_tile::Tile_Feature const& f,
                                           double tile_x, double tile_y,
                                           double scale_x, double scale_y)
{

    Geometry::command cmd;
    Geometry geoms(f,tile_x,tile_y,scale_x,scale_y);
    double x1, y1;
    switch (f.type())
    {
        case vector_tile::Tile_GeomType_POINT:
        {
            mapnik::geometry::multi_point mp;
            while ((cmd = geoms.next(x1, y1)) != Geometry::end) {
                mp.emplace_back(mapnik::geometry::point(x1,y1));
            }
            std::size_t num_points = mp.size();
            if (num_points == 1)
            {
                // return the single point
                return mapnik::geometry::geometry(std::move(mp[0]));;
            }
            else if (num_points > 1)
            {
                // return multipoint
                return mapnik::geometry::geometry(std::move(mp));;
            }
            break;
        }
        case vector_tile::Tile_GeomType_LINESTRING:
        {
            mapnik::geometry::multi_line_string mp;
            mp.emplace_back();
            bool first = true;
            while ((cmd = geoms.next(x1, y1)) != Geometry::end) {
                if (cmd == Geometry::move_to) {
                    if (first)
                    {
                        first = false;
                    }
                    else
                    {
                        mp.emplace_back();
                    }
                }
                mp.back().add_coord(x1,y1);
            }
            std::size_t num_lines = mp.size();
            if (num_lines == 1)
            {
                // return the single line
                return mapnik::geometry::geometry(std::move(mp[0]));;
            }
            else if (num_lines > 1)
            {
                // return multiline
                return mapnik::geometry::geometry(std::move(mp));;
            }
            break;
        }
        case vector_tile::Tile_GeomType_POLYGON:
        {
            // TODO - support for multipolygons
            //mapnik::geometry::multi_polygon;
            mapnik::geometry::polygon poly;
            std::vector<mapnik::geometry::linear_ring> rings;
            rings.emplace_back();
            double x2,y2;
            bool first = true;
            while ((cmd = geoms.next(x1, y1)) != Geometry::end) {
                if (cmd == Geometry::move_to)
                {
                    x2 = x1;
                    y2 = y1;
                    if (first)
                    {
                        first = false;
                    }
                    else
                    {
                        rings.emplace_back();
                    }
                }
                else if (cmd == Geometry::close)
                {
                    rings.back().add_coord(x2,y2);
                    continue;
                }
                rings.back().add_coord(x1,y1);
            }
            std::size_t num_rings = rings.size();
            if (num_rings == 1)
            {
                // return the single polygon
                mapnik::geometry::polygon poly;
                poly.set_exterior_ring(std::move(rings[0]));
                return mapnik::geometry::geometry(std::move(poly));
            }
            for (unsigned i = 0; i < num_rings;++i)
            {
                mapnik::geometry::polygon poly;
                if (i == 0)
                {
                    poly.set_exterior_ring(std::move(rings[i]));
                }
                else
                {
                    poly.add_hole(std::move(rings[i]));
                }
                return mapnik::geometry::geometry(std::move(poly));
            }
            return poly;
            break;
        }
        case vector_tile::Tile_GeomType_UNKNOWN:
        default:
        {
            throw std::runtime_error("unhandled geometry type during decoding");
            break;
        }
    }
    return mapnik::geometry::geometry();
}


Geometry::Geometry(vector_tile::Tile_Feature const& f,
                   double tile_x, double tile_y,
                   double scale_x, double scale_y)
    : f_(f),
      scale_x_(scale_x),
      scale_y_(scale_y),
      k(0),
      geoms_(f_.geometry_size()),
      cmd(1),
      length(0),
      x(tile_x), y(tile_y),
      ox(0), oy(0) {}

Geometry::command Geometry::next(double& rx, double& ry) {
    if (k < geoms_) {
        if (length == 0) {
            uint32_t cmd_length = static_cast<uint32_t>(f_.geometry(k++));
            cmd = cmd_length & 0x7;
            length = cmd_length >> 3;
        }

        --length;

        if (cmd == move_to || cmd == line_to) {
            int32_t dx = f_.geometry(k++);
            int32_t dy = f_.geometry(k++);
            dx = ((dx >> 1) ^ (-(dx & 1)));
            dy = ((dy >> 1) ^ (-(dy & 1)));
            x += (static_cast<double>(dx) / scale_x_);
            y += (static_cast<double>(dy) / scale_y_);
            rx = x;
            ry = y;
            if (cmd == move_to) {
                ox = x;
                oy = y;
                return move_to;
            } else {
                return line_to;
            }
        } else if (cmd == close) {
            rx = ox;
            ry = oy;
            return close;
        } else {
            fprintf(stderr, "unknown command: %d\n", cmd);
            return end;
        }
    } else {
        return end;
    }
}

}} // end ns


#endif // __MAPNIK_VECTOR_TILE_GEOMETRY_DECODER_H__
