#include "geometry.h"

namespace Faunus {
    Point ranunit(Random &rand, const Point &dir) {
        double r2;
        Point p;
        do {
            for (size_t i=0; i<3; i++)
                p[i] = ( rand()-0.5 ) * dir[i];
            r2 = p.squaredNorm();
        } while ( r2 > 0.25 );
        return p / std::sqrt(r2);
    }

    Point ranunit_polar(Random &rand) {
        return rtp2xyz( {1, 2*pc::pi*rand(), std::acos(2*rand()-1)} );
    }

    Point xyz2rtp(const Point &p, const Point &origin) {
        Point xyz = p - origin;
        double radius = xyz.norm();
        return {
            radius,
                std::atan2( xyz.y(), xyz.x() ),
                std::acos( xyz.z()/radius) };
    }

    Point rtp2xyz(const Point &rtp, const Point &origin) {
        return origin + rtp.x() * Point(
                std::cos(rtp.y()) * std::sin(rtp.z()),
                std::sin(rtp.y()) * std::sin(rtp.z()),
                std::cos(rtp.z()) );
    }

    namespace Geometry {

        GeometryBase::~GeometryBase() {}

         void from_json(const json &j, Chameleon &g) {
            g.from_json(j);
        }

        void to_json(json &j, const Chameleon &g) {
            g.to_json(j);
        }

        const std::map<std::string,Chameleon::Variant> Chameleon::names = {
            {
                {"cuboid", CUBOID},
                {"cylinder", CYLINDER},
                {"slit", SLIT},
                {"sphere", SPHERE}
            }
        };

        void Chameleon::setLength(const Point &l) {
            len = l;
            len_half = l*0.5;
            len_inv = l.cwiseInverse();
            c1 = l.y()/l.x();
            c2 = l.z()/l.x();
        }

        double Chameleon::getVolume(int dim) const {
            switch (type) {
                case SPHERE:   return 4*pc::pi/3*radius*radius*radius;
                case CUBOID:   return len.x()*len.y()*len.z();
                case SLIT:     return len.x()*len.y()*len.z() / pbc_disable;
                case CYLINDER: return pc::pi*radius*radius*len.z();
            }
        }

        Point Chameleon::setVolume(double V, VolumeMethod method) {
            if (type==CUBOID) {
                double x, alpha;
                Point s;
                switch (method) {
                    case ISOTROPIC:
                        x = std::cbrt( V / (c1*c2) ); // keep aspect ratio
                        s = { x, c1*x, c2*x };
                        setLength(s);
                        break;
                    case XY:
                        x = std::sqrt( V / (c1*len.z()) ); // keep xy aspect ratio
                        s = { x, c1*x, len.z() };
                        setLength(s);  // z is untouched
                        break;
                    case ISOCHORIC:
                        // z is scaled by 1/alpha/alpha, x and y are scaled by alpha
                        alpha = sqrt( V / pow(getVolume(), 1./3.) );
                        s = { alpha, alpha, 1/(alpha*alpha) };
                        setLength( len.cwiseProduct(s) );
                        return s;
                    default:
                        throw std::runtime_error("unknown volume scaling method");
                }
                assert( fabs(getVolume()-V)<1e-6 );
                return s.cwiseQuotient(len); // this will scale any point to new volume
            }

            if (type==CYLINDER and method==ISOTROPIC) {
                double rold = radius;
                radius = std::sqrt( V / (pc::pi * len.z()) );
                setLength( { pbc_disable*2*radius, pbc_disable*2*radius, len.z() } );
                assert( fabs(getVolume()-V)<1e-6 );
                return Point(radius/rold, radius/rold, 1);
            }

            if (type==SPHERE and method==ISOTROPIC) {
                double rold = radius;
                radius = std::cbrt(3*V/(4*pc::pi));
                setLength( pbc_disable*2*Point(radius,radius,radius) ); // disable min. image in xyz
                assert( fabs(getVolume()-V)<1e-6 );
                return Point().setConstant(radius/rold);
            }

            throw std::runtime_error("unsupported volume move for geometry");

            return {1,1,1};
        }

        Point Chameleon::getLength() const {
            switch (type) {
                case CUBOID:   return len;
                case SLIT:     return {len.x(), len.y(), len.z() / pbc_disable};
                case SPHERE:   return {2*radius,2*radius,2*radius};
                case CYLINDER: return {2*radius,2*radius,len.z()};
            }
        } //!< Enscribe geometry in cuboid

        void Chameleon::randompos(Point &m, Random &rand) const {
            double r2 = radius*radius, d=2*radius;
            switch (type) {
                case SPHERE:
                    do {
                        m.x() = (rand() - 0.5) * d;
                        m.y() = (rand() - 0.5) * d;
                        m.z() = (rand() - 0.5) * d;
                    } while ( m.squaredNorm() > r2 );
                    break;
                case CYLINDER:
                    m.z() = (rand()-0.5) * len.z();
                    do {
                        m.x() = (rand()-0.5) * d;
                        m.y() = (rand()-0.5) * d;
                        d = m.x()*m.x() + m.y()*m.y();
                    } while ( d>r2 );
                    break;
                case SLIT:
                    m.x() = (rand()-0.5) * len.x();
                    m.y() = (rand()-0.5) * len.y();
                    m.z() = (rand()-0.5) * len.z() / pbc_disable;
                case CUBOID:
                    m.x() = (rand()-0.5) * len.x();
                    m.y() = (rand()-0.5) * len.y();
                    m.z() = (rand()-0.5) * len.z();
                    break;
            }
        }

        bool Chameleon::collision(const Point &a, double r) const {
            assert(std::fabs(r)<1e-6 && "collision test w. radius unimplemented.");
            double r2 = radius*radius;
            switch (type) {
                case SPHERE:
                    return (a.squaredNorm()>r2) ? true : false;
                    break;
                case CYLINDER:
                    if ( std::fabs(a.z()) > len_half.z()) return true;
                    if ( a.x()*a.x() + a.y()*a.y() > r2 ) return true;
                    break;
                case SLIT:
                    if ( std::fabs(a.x()) > len_half.x()) return true;
                    if ( std::fabs(a.y()) > len_half.y()) return true;
                    if ( std::fabs(a.z()) > len_half.z() / pbc_disable ) return true;
                    break;
                case CUBOID:
                    if ( std::fabs(a.x()) > len_half.x()) return true;
                    if ( std::fabs(a.y()) > len_half.y()) return true;
                    if ( std::fabs(a.z()) > len_half.z()) return true;
                    break;
            }
            return false;
        }

        void Chameleon::from_json(const json &j) {
            auto it = names.find( j.at("type").get<std::string>() );
            if (it==names.end())
                throw std::runtime_error("unknown geometry");

            name = it->first;
            type = it->second;
            len.setZero();

            if ( type == CUBOID or type == SLIT ) {
                auto m = j.at("length");
                if (m.is_number()) {
                    double l = m.get<double>();
                    setLength( {l,l,l} );
                } else if (m.is_array())
                    if (m.size()==3)
                        setLength( m.get<Point>() );
            }

            if ( type == SPHERE or type == CYLINDER )
                radius = j.at("radius").get<double>();

            if (type == SLIT)
                setLength( {len.x(), len.y(), pbc_disable*len.z() } );  // disable min. image in z

            if (type == SPHERE)
                setLength( pbc_disable*2*Point(radius,radius,radius) ); // disable min. image in xyz

            if (type == CYLINDER) {
                len.z() = j.at("length").get<double>();
                setLength( {2*radius*pbc_disable, 2*radius*pbc_disable, len.z() } ); // disable min. image in xy
            }
        }

        void Chameleon::to_json(json &j) const {
            assert(!name.empty());
            switch (type) {
                case SPHERE:
                    j = {{"radius",radius}};
                    break;
                case CYLINDER:
                    j = {{"radius",radius}, {"length",len.z()}};
                    break;
                case SLIT:
                    j = {{"length",Point(len.x(), len.y(), len.z() / pbc_disable)}};
                    break;
                case CUBOID:
                    j = {{"length",len}};
                    break;
            }
            j["type"] = name;
        }

    } // end of Geometry namespace

} // end of Faunus namespace
