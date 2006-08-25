/**
 * @file inbox.cc
 * Force atoms in the simulation box
 */

/**
 * @page programs Program Documentation
 *
 * @anchor inbox
 * @section inbox Force atoms in the simulation box
 * @author @ref AG
 * @date 24.8.2006
 *
 * Even though all GROMOS programs correct for periodic boundary conditions
 * whenever necessary, it can sometimes be quite cumbersome to display a
 * simulation box that contains all molecules at clear distance from one
 * another. For simulations containing one or a few solute molecules, program
 * frameout in combination with the proper gather method will be sufficient,
 * but especially in molecular systems consisting of many solute molecules, it
 * may happen that no settings exactly work.
 *
 * Program inbox, forces all solute and solvent atoms to be inside a single
 * computational box, regardless of their connectivity and relative distances
 * to other atoms. Before applying the box boundaries, the molecular system may
 * be shifted by a user specified vector. The box will be roughly centered
 * around the centre of geometry of all solute atoms.
 *
 * <b>arguments:</b>
 * <table border=0 cellpadding=0>
 * <tr><td> \@topo</td><td>&lt;topology&gt; </td></tr>
 * <tr><td> \@pbc</td><td>&lt;boundary type&gt; </td></tr>
 * <tr><td> \@traj</td><td>&lt;trajectory files&gt; </td></tr>
 * <tr><td> [\@shift</td><td>&lt;vector to shift&gt;] </td></tr>
 * </table>
 *
 *
 * Example:
 * @verbatim
  inbox
    @topo   ex.top
    @pbc    r
    @traj   exref.coo
    @shift  0.6 -0.5 0.4
 @endverbatim
 *
 * <hr>
 */

#include <cassert>
#include <vector>
#include <iomanip>
#include <iostream>

#include "../src/args/Arguments.h"
#include "../src/args/BoundaryParser.h"
#include "../src/args/GatherParser.h"
#include "../src/gio/InG96.h"
#include "../src/gio/OutPdb.h"
#include "../src/gcore/System.h"
#include "../src/gcore/Molecule.h"
#include "../src/gcore/MoleculeTopology.h"
#include "../src/gcore/Solvent.h"
#include "../src/gcore/SolventTopology.h"
#include "../src/gcore/AtomTopology.h"
#include "../src/gcore/Box.h"
#include "../src/gio/InTopology.h"
#include "../src/bound/Boundary.h"
#include "../src/bound/TruncOct.h"
#include "../src/gmath/Vec.h"

using namespace gcore;
using namespace gio;
using namespace bound;
using namespace args;

using namespace std;

int main(int argc, char **argv){

  char *knowns[] = {"topo", "pbc", "traj", "shift"};
  int nknowns = 4;

  string usage = "# " + string(argv[0]);
  usage += "\n\t@topo    <topology>\n";
  usage += "\t@pbc     <boundary type>\n";
  usage += "\t@traj    <trajectory files>\n";
  usage += "\t[@shift  <vector to shift>]\n";
  
 
  try{
    Arguments args(argc, argv, nknowns, knowns, usage);

    //  read topology
    InTopology *it;
    it = new InTopology(args["topo"]);
    System sys(it->system());
    
    // read shifting
    Vec shift(0.0,0.0,0.0);
    
    Arguments::const_iterator iter=args.lower_bound("shift"), to=args.upper_bound("shift");
    if(iter!=to){
      shift[0]=atof(iter->second.c_str());
      ++iter;
    }
    if(iter!=to){
       shift[1]=atof(iter->second.c_str());
      ++iter;
    }
    if(iter!=to)       
      shift[2]=atof(iter->second.c_str());

    // parse boundary conditions
    Boundary *pbc;
    pbc = BoundaryParser::boundary(sys, args);
    //Boundary::MemPtr gathmethod = args::GatherParser::parse(args);

    // define input coordinate
    InG96 ic;
    ic.open(args["traj"].c_str());
    ic.select("ALL");
 
    // output
    OutCoordinates *oc;
    oc = new OutPdb();
    
    oc->open(cout);
    oc->select("ALL");  
    oc->writeTitle(ic.title());
      
   
    // loop over single trajectory
    while(!ic.eof()){
      ic >> sys;
      Vec origin(sys.box()[0], sys.box()[1], sys.box()[2]);
      origin/=2;
      
      for(int i=0;i<sys.numMolecules(); i++){
        Vec cog(0.0,0.0,0.0);
	
	for(int j=0; j < sys.mol(i).numAtoms(); j++){
          sys.mol(i).pos(j)+=shift;
	  
	  cog+=sys.mol(i).pos(j);
	}
	cog/=sys.mol(i).numAtoms();
	
	cog=pbc->nearestImage(origin,cog, sys.box());
	for(int j=0; j<sys.mol(i).numAtoms(); j++){
	  sys.mol(i).pos(j)=pbc->nearestImage(cog, 
					      sys.mol(i).pos(j), 
					      sys.box());
	}
	 
	  

      }
      for(int i=0; i<sys.sol(0).numPos(); 
	  i+=sys.sol(0).topology().numAtoms()){
	sys.sol(0).pos(i)+=shift;
	
        sys.sol(0).pos(i)=pbc->nearestImage(origin, 
					    sys.sol(0).pos(i), 
					    sys.box());
	
	for(int j=1; j<sys.sol(0).topology().numAtoms(); j++){
	  sys.sol(0).pos(i+j)+=shift;
	  
	  sys.sol(0).pos(i+j)=pbc->nearestImage(sys.sol(0).pos(i), 
						sys.sol(0).pos(i+j), 
						sys.box());
	}
      }
      
      
      //pbc->gather();
      
      //(*pbc.*gathmethod)();
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
