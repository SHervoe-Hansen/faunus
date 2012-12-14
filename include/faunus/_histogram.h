#ifndef FAU_HISTOGRAM_H
#define FAU_HISTOGRAM_H

#ifndef SWIG
#include <faunus/common.h>
#include <faunus/average.h>
#include <faunus/xytable.h>
#include <faunus/point.h>
#endif

namespace Faunus {
  /*
     class xi_convert {
     private:
     double dxhalf;
     public:
     bool interpolate;
     double xmin, xmax, dx;
     xi_convert(double min, double max, double delta, bool intp=false) {
     }
     int x2i(const double &x) { return std::floor( (x-xmin)/dx); }

     double i2x(const int &i) {
     }
     };
     */

  /*!
   * \brief Histogram class
   * \author Mikael Lund
   * \code
   * histogram h(0.1,-5,5);
   * h.add(-4.9);
   * h.add(4);
   * h.add(-5.0);
   * h.write("sletmig");
   * float val=h.get(4); // val=0.3333
   * \endcode
   */
  class Histogram : protected xytable<float,unsigned long int> {
    protected:
      unsigned long int cnt;
      float xmaxi,xmini;  // ugly solution!
    public:
      Histogram(float, float, float);
      virtual ~Histogram();
      void reset(float, float, float);    //!< Clear data
      string comment;                     //!< User defined comment
      void add(float);
      void write(string);
      void save(string);
      virtual float get(float);
      vector<double> xvec();              //!< For python interface
      vector<double> yvec();              //!< For python interface
  };

  /*!
   *  \brief Atomic RDF between two groups
   *
   *  \author Mikael Lund
   *  \date Lund 2009
   *  \note This function does not include volume weighting and the
   *        CM's of the two groups should be fixed along one axis
   *        cf. Faunus::dualmove
   */
  class atomicRdf : public Histogram {
    public:
      atomicRdf(float=.5, float=0);
      //void update(vector<particle> &, group &, group &); //!< Update histogram vector
  };

  /*
   * \brief Particle profile base class
   * \author Mikael Lund
   * \date Canberra, 2008
   *
   * This is a base class for analysing particle distributions
   * along some one-dimensional coordinate. Derived classes should
   * implement an add() function that, given, a particle adds it
   * to a histogram if certain criteria are fulfulled.
   */
  class profile : protected xytable<float,unsigned long int>{
    protected:
      unsigned int cnt;
      virtual float volume(double)=0; //!< Get volume at coordinate
    public:
      profile(float, float, float=.5);
      virtual ~profile();
      virtual void add(particle &)=0; //!< Add a particle
      void update(vector<particle> &);//!< Search for and add particles
      float conc(float);              //!< Get concentration at coordinate
      bool write(string);             //!< Print distribution
  };

  /*!\brief Cummulative sum around a central particle
   * \author Mikael Lund
   * \date Canberra, 2008
   * \warning The central particle is passed through the constructor and
   *          it is assumed that the particle (usually in a vector) is not
   *          reallocated. Hence, do NOT modify the particle vector after
   *          having called the constructor.
   */
  class CummulativeSum : public profile {
    protected:
      particle *origo;         //!< Central particle (coordinate origo)
      float volume(double);
    public:
      unsigned char id;        //!< Particle type to analyse
      CummulativeSum(unsigned char, particle &, float, float=.5);
      //void add(particle &);
  };

  /*
   * \brief Cylindrical particle distribution
   * \author Mikael Lund
   * \date Canberra, 2008
   *
   * Calculates the particle density in a cylinder around
   * the z-axis.
   */
  class cylindric_profile : public profile {
    protected:
      float volume(double);
    public:
      double r;                //!< Radius of the cylinder
      unsigned char id;        //!< Particle type to analyse
      cylindric_profile(float, unsigned char, float, float, float=.5);
      void add(particle &);
  };

  class radial_profile : protected xytable<float,unsigned long int>{
    protected:
      unsigned int cnt;
      float volume(float);            //!< Get area at coordinate
      Point origo;
    public:
      radial_profile( float, float, float=.5);
      void add(particle &);           //!< Add a particle
      void add(Point &, Point &);              //!< Add a particle
      void update(vector<particle> &);//!< Search for and add particles
      float conc(float);              //!< Get concentration at coordinate
      bool write(string);             //!< Print distribution
      unsigned char id;
  };
}//namespace
#endif