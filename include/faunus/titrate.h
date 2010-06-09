#ifndef FAU_titrate_h
#define FAU_titrate_h

#include <vector>
#include <algorithm>
#include <iostream>
#include "faunus/species.h"
#include "faunus/point.h"
#include "faunus/group.h"
#include "faunus/average.h"
#include "faunus/ensemble.h"

namespace Faunus {

  /*! \brief Class to perform proton titration of molecules
   *  \author Mikael Lund
   *  \todo Too much is public! Documentation is bad.
   */
  class titrate {
    protected:
      vector<short int> sites, protons, neutrons;
      enum keywords {PROTONATED,DEPROTONATED,ANY,NOACID};
      struct action {
        keywords action;
        short int site;        
        short int proton;    
      };
      action exchange(vector<particle> &);
      action exchange(vector<particle> &, action &);
      short int random(vector<short int> &); //!< Pick a random item in a vector
      average<float> nprot;                  //!< Average number of protons. Updated with titrate::samplesites
      vector<average <double> > q;           //!< Stores the average charges of sites from titrate::sites
      action takeFromBulk(vector<particle> &, short int, short int=-1);
      action moveToBulk(vector<particle> &, short int, short int=-1);
      keywords status( vector<particle> &, short int );
      group sort(group &);
      virtual double energy(vector<particle> &, double, action &);

    public:
      double ph;                                //!< System pH
      titrate(double);
      titrate(vector<particle> &, group &, double);
      void init(vector<particle> &, group &);   //!< Locate and initialize sites and protons
      double sumsites();                        //!< Calculates total charge of titrateable sites
      void samplesites(vector<particle> &);     //!< Updates the average charge vector titrate::q
      void showsites(vector<particle> &);       //!< Print average charges of titrateable sites
      double applycharges(vector<particle> &);  //!< Copy average charges to particles in the particle vector
      double avgcharge(vector<particle> &, unsigned int);//!< Returns average charge of a particle
      //bool savestate(string);                   //!< Save titration state of the system
      //bool loadstate(string);                   //!< Load titration state of the system
      void infos();
  };

  /*! \brief Class to perform proton titration of molecules without PROTON/NEUTRON shuffeling.
   *  \author Andre, Mikael
   */
  class titrate_implicit {
    protected:
      vector<average <double> > zavg;       //!< Stores the average charges of sites from titrate::sites
    public:
      enum keys {PROTONATED,DEPROTONATED};
      keys recent;                          //!< Attempted change of state in the Markov chain.
      vector<unsigned int> sites;           //!< Titratable sites
      double ph;                            //!< Solution pH value
      titrate_implicit(vector<particle> &, double);
      titrate_implicit(container&, double);
      int exchange(vector<particle> &, int=-1);
      virtual double energy(vector<particle> &, double, int);
      unsigned int random();                //!< Pick a random titratable site
      string info();                        //!< Returns information string
  };

  /*! \brief Class to perform grand canonical proton titration. The process is couples with 
   * the exchange of a nother ionic pair with known chemical potential. It is assumed 
   * that the charges of the exchanged particles are mono valent.
   */
  class titrate_gc : public titrate_implicit {
    private:
    public:
      grandcanonical *gcPtr;                //!< Pointer to grandcanonical rotines
      string          nameA;                //!< Name of ion coupled with proton exchange
      int             nA;
      double          energy(container &, double &, int &);
      titrate_gc(container&,inputfile&,grandcanonical&); //!< Constructor
      string info();                        //!< Returns information string
      string info(container&);              //!< Returns expanded information string
  };
}
#endif
