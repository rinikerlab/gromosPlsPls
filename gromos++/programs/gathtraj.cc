/* gathtraj.cc
 * write a gathered trajectory to stdout.
 * Vincent Kraeutler, March 2002.
*/

#include "../src/args/Arguments.h"
#include "../src/args/BoundaryParser.h"
#include "../src/gio/InG96.h"
#include "../src/gio/OutG96.h"
#include "../src/gcore/System.h"
#include "../src/gcore/Molecule.h"
#include "../src/gcore/MoleculeTopology.h"
#include "../src/gcore/AtomTopology.h"
#include "../src/gio/InTopology.h"
#include "../src/bound/Boundary.h"
#include "../src/bound/TruncOct.h"
#include "../src/gmath/Vec.h"
#include <vector>
#include <iomanip>
#include <iostream>

using namespace gcore;
using namespace gio;
using namespace bound;
using namespace args;

// Wrappers for the gathering methods.
void gather(Boundary *b, gmath::Vec dummy){
  b->gather();
}
void gathergr(Boundary *b, gmath::Vec dummy){
  b->gathergr();
}
void coggather(Boundary *b, gmath::Vec centerOfGravity){
  b->coggather(centerOfGravity);
}

int main(int argc, char **argv){

  char *knowns[] = {"topo", "pbc", "gath", "cog", "traj"};
  int nknowns = 5;

  string usage = argv[0];
  usage += "\n\t@topo <topology> (defaults to \"topo\")\n";
  usage += "\t@pbc <boundary type> (defaults to \"t\")\n";
  usage += "\t@gath <gathering method> (defaults to \"gather\")\n";
  usage += "\t@cog <centering vector> (ignored when gath != \"coggather\")\n";
  usage += "\t@traj <trajectory files> (defaults to \"traj\")\n";
 
  try{
    Arguments args(argc, argv, nknowns, knowns, usage);

    //  read topology
    InTopology *it;
    try{
      args.check("topo", 1);
      it = new InTopology(args["topo"]);
    }
    catch (const gromos::Exception &e){
      it = new InTopology("topo"); 
    }
    System sys(it->system());
    
    // parse boundary conditions
    Boundary *pbc;
    try{
      args.check("pbc", 1);
      pbc = BoundaryParser::boundary(sys, args);
    }
    catch (const gromos::Exception &e){
      pbc = new TruncOct(&sys);
    }

    // check gathering method
    gmath::Vec centeringVector;
    void (*gatherMethod)(bound::Boundary *, gmath::Vec cVector) = NULL;
    string gath;
    try{
      args.check("gath", 1);
      gath = args["gath"].c_str();
    }
    catch (const gromos::Exception &e){ 
      gath = "gather";
    }
    if(gath == "gather")
      gatherMethod = gather;
    else if(gath == "gathergr")
      gatherMethod = gathergr;
    else if(gath == "coggather"){
      gatherMethod = coggather;
      // fill in the coordinates of the cog-vector
      args.check("cog", 3);
      Arguments::const_iterator coord = args.lower_bound("cog");
      for(int i = 0; i < 3; i++){
        centeringVector[i] = atof(coord->second.c_str());
        coord++;
      }
    }
    
    // define input coordinate
    InG96 ic;
    try {
      ic.open(args["traj"].c_str());
    }
    catch (const gromos::Exception &e){
      ic.open("traj");
    }

    // output
    OutCoordinates *oc;
    oc = new OutG96();
    oc->open(cout);  
    oc->writeTitle(ic.title());
      
    // loop over single trajectory
    while(!ic.eof()){
      ic >> sys;
      gatherMethod(pbc, centeringVector);
      *oc << sys;
    }

    ic.close();
    oc->close();
  }
  catch (const gromos::Exception &e){
    cerr << e.what() << endl;
    exit(1);
  }
  return 0;
}
