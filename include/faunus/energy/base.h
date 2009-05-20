#ifndef FAU_ENERGYBASE_H
#define FAU_ENERGYBASE_H

#include "faunus/common.h"
#include "faunus/group.h"
#include "faunus/inputfile.h"

namespace Faunus {
  /*!
   *  \brief Base class for interactions between particles and groups
   *  \author Mikael Lund
   *
   *  Calculates interaction energies between particles and groups. The
   *  pair potential is specified as a template type which allows inlining.
   *  Unless otherwise specified, all energies will be returned in units of \b kT.
   */
  class energybase {
    protected:
      string name;
    public:
      double tokT;
      energybase(double f) { tokT=f; }
      virtual double energy(const particle &, const particle &)=0;                       //!< particle<->particle (slow!)
      virtual double energy(const vector<particle> &, const particle &)=0;               //!< all<->external particle
      virtual double energy(const vector<particle> &, int)=0;                            //!< all<->particle i.
      virtual double energy(const vector<particle> &, const group &)=0;                  //!< all<->group.
      virtual double energy(const vector<particle> &)=0;                                 //!< all<->all (System energy).
      virtual double energy(const vector<particle> &, const group &, const group &)=0;   //!< group<->group.
      virtual double energy(const vector<particle> &, const group &, int)=0;             //!< group<->particle i.
      virtual double energy(const vector<particle> &, const group &, const particle &)=0;//!< group<->external particle.
      virtual double potential(const vector<particle> &, unsigned short)=0;              //!< Electric potential at j'th particle
      virtual double potential(const vector<particle> &, point)=0;                       //!< Electric potential in point
      virtual double internal(const vector<particle> &, const group &, int=1)=0;         //!< internal energy in group
      virtual double pot(const vector<particle> &, const point &)=0;                     //!< Electrostatic potential in a point
      virtual double dipdip(const point &, const point &, double)=0;                     //!< Dipole-dipole energy.
      virtual double iondip(const point &, double, double)=0;                            //!< Ion-dipole energy.
      virtual double force(container &, particle, particle, double, double=.5)=0;        //!< Force vector
      virtual void forceall(container &, vector<point> &)=0;
      virtual double u_monomer(const vector<particle> &, const polymer &, unsigned int)=0; //!< all<->monomer in polymer

      string info() {
        std::ostringstream o;
        o << endl
          << "# ENERGY EVALUATION:" << endl
          << "#   Scheme:             " << name << endl;
        return o.str();
      }
  };

  /*!
   * \brief Implementation of all energy functions
   * \author Mikael Lund
   *
   * This is an expression template that constructs the
   * energy functions of a particular pair potential. All
   * returned energies are in units of kT.
   *
   * \code
   * inputfile in("input.conf");
   * interaction<pot_coulomb> pot(in);
   * pot.energy(...);
   * \endcode
   */
  template<class T> class interaction : public energybase {
    public:
      T pair; //!< An instance of the pair-potential.
      interaction(inputfile &in) : pair(in), energybase(pair.f) {
        name="Full N^2";
        tokT=pair.f;
      };

      double energy(const particle &a, const particle &b) {
        return pair.pairpot(a,b) * pair.f;
      }

      double energy(const vector<particle> &p, int j) {
        int i,ps=p.size();
        double u=0;
        if (j>0)
#pragma omp parallel for reduction (+:u)
          for (i=0; i<j; i++)
            u+=pair.pairpot( p[i],p[j] );
#pragma omp parallel for reduction (+:u)
        for (i=j+1; i<ps; i++)
          u+=pair.pairpot( p[i],p[j] );
        return pair.f*u;
      }

      double energy(const vector<particle> &p, const group &g) {
        int n=g.end+1, psize=p.size();
        double u=0;
        for (int i=g.beg; i<n; ++i) {
          if (g.beg>0)
#pragma omp parallel for reduction (+:u)
            for (int j=0; j<g.beg; j++)
              u += pair.pairpot(p[i],p[j]);
#pragma omp parallel for reduction (+:u)
          for (int j=n; j<psize; j++)
            u += pair.pairpot(p[i],p[j]);
        }
        return pair.f*u;
      }

      double energy(const vector<particle> &p, const group &g, int j) {
        double u=0;
        int len=g.end+1;
        if (g.find(j)==true) {   //avoid self-interaction...
          for (int i=g.beg; i<j; i++)
            u+=pair.pairpot(p[i],p[j]);
          for (int i=j+1; i<len; i++)
            u+=pair.pairpot(p[i],p[j]);
        } else                        //simple - j not in g
          for (int i=g.beg; i<len; i++)
            u+=pair.pairpot(p[i],p[j]);
        return pair.f*u;  
      }

      double energy(const vector<particle> &p, const group &g, const particle &a) {
        if (g.beg==-1) return 0;
        double u=0;
        int i,n=g.end+1;
        //#pragma omp parallel for reduction (+:u)
        for (i=g.beg; i<n; i++)
          u+=pair.pairpot(a, p[i]); 
        return pair.f*u;
      }

      double energy(const vector<particle> &p, const vector<macromolecule> &g) {
        double u=0;
        int k,j=g.size(),t=p.size();
        for (int l=0; l<j; l++) {
          k=g[l].end+1;
          for (int i=g[l].beg; i<k; i++) {
            for (int s=(g[l].end+1); s<t; s++) {
              u+=pair.pairpot(p[i],p[s]);
            }
          }
        }
        return pair.f*u;
      }

      double energy(const vector<particle> &p) {
        double u=0;
        int n = p.size();
#pragma omp parallel for reduction (+:u)
        for (int i=0; i<n-1; ++i)
          for (int j=i+1; j<n; ++j)
            u+=pair.pairpot(p[i], p[j]);
        return pair.f*u;
      }

      double energy(const vector<particle> &p, const group &g1, const group &g2) {
        double u=0;
        int ilen=g1.end+1, jlen=g2.end+1;
#pragma omp parallel for reduction (+:u)
        for (int i=g1.beg; i<ilen; i++)
          for (int j=g2.beg; j<jlen; j++)
            u += pair.pairpot(p[i],p[j]);
        return pair.f*u;
      }

      /*!
       * ...between the two dipoles a and b, separated by the
       * distance r.
       * \f$ \beta u(r) = l_B \frac{a_x b_x + a_y b_y - 2a_z b_z  }{r^3}\f$
       */
      double dipdip(const point &a, const point &b, double r) {
        return pair.f*( a.x*b.x + a.y*b.y - 2*a.z*b.z )/(r*r*r);
      }
      double iondip(const point &a, double q, double r) { return -pair.f*q*a.z/(r*r); }

      // Total electrostatic potential in a point
      double pot(const vector<particle> &p, const point &a) {
        double u=0;
        int i,n=p.size();  
        for (i=0; i<n; i++)
          u+=p[i].charge/sqrt(pair.sqdist(a,p[i]));
        return pair.f*u;
      }

      virtual double internal(const vector<particle> &p, const group &g, int step=1) {
        if (g.beg==-1) return 0;
        double u=0;
        int n=g.end+1;
        for (int i=g.beg; i<n-step; i++)
          for (int j=g.beg+step*((i-g.beg)/step+1); j<n; j++)
            u+=pair.pairpot(p[i],p[j]);
        return pair.f*u;
      }

      double energy(const vector<particle> &p, const particle &a) {
        double u=0;
        int i,n=p.size();
        for (i=0; i<n; i++)
          u+=pair.pairpot(p[i], a);
        return pair.f*u;
      }

      /*! \note If the charge of the j'th particle is 0, ZERO will be returned!
       *  \return \f$ \phi_j = \sum_{i\neq j}^{N} \frac{l_B z_i}{r_{ij}} \f$
       *  \param p Particle vector
       *  \param j The electric potential will be calculated in the point of this particle
       *  \todo Respect cell boundaries
       */
      double potential(const vector<particle> &p, unsigned short j) {
        if (abs(p[j].charge)<1e-6)
          return 0;
        double u=0;
        int i,n=p.size();
        for (i=0; i<j; ++i)
          u+=p[i].charge/sqrt(pair.sqdist(p[i],p[j]));
        for (i=j+1; i<n; ++i)
          u+=p[i].charge/sqrt(pair.sqdist(p[i],p[j]));
        return pair.f*u;
      }

      double potential(const vector<particle> &p, point a) {
        double phi=0;
        for (int i=0; i<p.size(); ++i)
          phi+=p[i].charge/sqrt( pair.sqdist(a,p[i]) );
        return pair.f*phi;
      }

      void forceall(container &c, vector<point> &f) {
        point r;
        double ff;
        int n=c.p.size();
        f.resize(n);
        for (int i=0; i<n; i++)
          f[i].clear();
        for (int i=0; i<n-1; i++)
          for (int j=i+1; j<n; j++)
          {
            r=c.vdist(c.p[i], c.p[j]);
            ff=c.p[i].charge*c.p[j].charge/(r.x*r.x+r.y*r.y+r.z*r.z);
            f[i].x+=ff*r.x;
            f[i].y+=ff*r.y;
            f[i].z+=ff*r.z;
            f[j].x-=ff*r.x;
            f[j].y-=ff*r.y;
            f[j].z-=ff*r.z;
          }
      }

      double force(container &c, particle a, particle b, double r, double dr) {
        double forward,center;
        a.x=a.y=a.z=b.y=b.z=0;
        b.x=r+dr;
        forward=pair.pairpot(a,b);
        b.x=r-dr;
        center=pair.pairpot(a,b);
        return -(forward-center)/(2*dr);
      }

      string info() {
        std::ostringstream o;
        o << energybase::info()
          << "#   Pair potential:" << endl << pair.info();
        return o.str();
      }
  };
}//namespace
#endif
