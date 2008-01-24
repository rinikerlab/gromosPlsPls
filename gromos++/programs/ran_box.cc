/**
 * @file ran_box.cc
 * Create a condensed phase system of any composition
 */

/**
 * @page programs Program Documentation
 *
 * @anchor ran_box
 * @section ran_box Create a condensed phase system of any composition
 * @author @ref dt
 * @date 7-6-07
 *
 * When simulating a molecular liquid, a starting configuration for the solvent
 * molecules has to be generated. Program ran_box generates a starting
 * configuration for the simulation of mixtures consisting of an unlimited 
 * number of components. The molecules are randomly placed in a cubic or a
 * truncated octahedron box, in a random orientation. Note that for the
 * generation of a starting configuration for the simulation of pure liquids
 * and binary mixtures, the programs @ref build_box and @ref bin_box can
 * alternatively be used (see sections V-2.9 and V-2.11, respectively).
 *
 * <b>arguments:</b>
 * <table border=0 cellpadding=0>
 * <tr><td> \@topo</td><td>&lt;topologies of single molecule for each molecule type: topo1 topo2 ...&gt; </td></tr>
 * <tr><td> \@pbc</td><td>&lt;boundary type&gt; </td></tr>
 * <tr><td> \@pos</td><td>&lt;coordinates of single molecule for each molecule type: pos1 pos2 ...&gt; </td></tr>
 * <tr><td> \@nsm</td><td>&lt;number of molecules for each molecule type: nsm1 nsm2 ...&gt; </td></tr>
 * <tr><td> \@dens</td><td>&lt;density of liquid (kg/m^3)&gt; </td></tr>
 * <tr><td> \@thresh</td><td>&lt;threshold distance in overlap check; default: 0.20 nm&gt; </td></tr>
 * <tr><td> \@layer</td><td>(create molecules in layers (along z axis)) </td></tr>
 * <tr><td> \@boxsize</td><td>&lt;boxsize&gt; </td></tr>
 * <tr><td> \@fixfirst</td><td>(do not rotate / shift first molecule) </td></tr>
 * <tr><td> \@seed</td><td>&lt;random number genererator seed&gt; </td></tr>
 * </table>
 *
 *
 * Example:
 * @verbatim
  ran_box
    @topo       ic4.top   urea.top   h2o.top
    @pos        ic4.g96   urea.g96   h2o.g96
    @nsm        1         153        847
    @pdb        r     
    @dens       1000
    @thresh     0.2
    @seed       100477
 @endverbatim
 *
 * <hr>
 */
#include <cassert>
#include <vector>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cmath>
#include <iostream>
#include <unistd.h>

#include "../src/args/Arguments.h"
#include "../src/args/BoundaryParser.h"
#include "../src/args/GatherParser.h"
#include "../src/bound/TruncOct.h"
#include "../src/bound/Vacuum.h"
#include "../src/bound/RectBox.h"
#include "../src/bound/Boundary.h"
#include "../src/fit/PositionUtils.h"
#include "../src/gio/InG96.h"
#include "../src/gio/OutG96S.h"
#include "../src/gcore/System.h"
#include "../src/gcore/Molecule.h"
#include "../src/gcore/MoleculeTopology.h"
#include "../src/gcore/AtomTopology.h"
#include "../src/gcore/Box.h"
#include "../src/gio/InTopology.h"
#include "../src/gmath/Vec.h"
#include "../src/gmath/Matrix.h"

using namespace std;
using namespace bound;
using namespace gcore;
using namespace gio;
using namespace fit;
using namespace gmath;
using namespace args;


const double pi = acos(-1.0);
const double fac_amu2kg = 1.66056;

// declarations
bool overlap(System const & sys, double threshhold, Boundary * pbc);
int place_random(System & sys, Boundary * pbc, int layer = 0, int nlayer = 1);

int main(int argc, char **argv){
  
  char *knowns[] = {"topo", "pbc", "pos", "nsm", "dens", "thresh", "layer",
		    "boxsize", "fixfirst", "seed"};
  int nknowns = 10;
  
  string usage = "# " + string(argv[0]);
  usage += "\n\t@topo     <topologies of single molecule for each molecule type: topo1 topo2 ...>\n";
  usage += "\t@pbc      <boundary type>\n";
  usage += "\t@pos      <coordinates of single molecule for each molecule type: pos1 pos2 ...>\n";
  usage += "\t@nsm      <number of molecules for each molecule type: nsm1 nsm2 ...>\n";
  usage += "\t@dens     <density of liquid (kg/m^3)>\n";
  usage += "\t@thresh   <threshold distance in overlap check; default: 0.20 nm>\n";
  usage += "\t@layer    (create molecules in layers (along z axis))\n";
  usage += "\t@boxsize  <boxsize>\n";
  usage += "\t@fixfirst (do not rotate / shift first molecule)\n";
  usage += "\t@seed     <random number genererator seed>\n";
  
  
  try{
    Arguments args(argc, argv, nknowns, knowns, usage);

    if (args.count("seed") > 0){
      std::istringstream is(args["seed"]);
      unsigned int s;
      is >> s;
      srand(s);
    }
    else{
      srand(time(NULL));
    }
    
    // reading input and setting some values
    if ( args.count("topo") != args.count("pos") ||  args.count("topo") != args.count("nsm") ) {
      throw gromos::Exception("ran_box", "Check the number of arguments for @topo, @pos and @nsm");
    }

    if (args.count("boxsize") >= 0 && args.count("dens") >=0){
      throw Arguments::Exception("don't specify both boxsize and density!");
    }
    if (args.count("boxsize") == 0){
      throw Arguments::Exception("boxsize: <length> for cubic or "
				 "<len_x len_y len_z> for rectangular box!");
    }
    
    args.check("nsm",1);
    vector<int> nsm;
    Arguments::const_iterator iter=args.lower_bound("nsm");
    while(iter!=args.upper_bound("nsm")){
      nsm.push_back(atoi(iter->second.c_str()));
      ++iter;
    }

    args.check("topo",1);
    vector<string> tops;
    iter=args.lower_bound("topo");
    while(iter!=args.upper_bound("topo")){
      tops.push_back(iter->second.c_str());
      ++iter;
    }
    
    args.check("pos",1);
    vector<string> insxs;
    iter=args.lower_bound("pos");
    while(iter!=args.upper_bound("pos")){
      insxs.push_back(iter->second.c_str());
      ++iter;
    }    
    
    bool fixfirst = false;
    {
      if (args.count("fixfirst") >= 0){
	fixfirst = true;
	if (nsm[0] != 1)
	  throw Arguments::Exception("fixfirst only allowed for a single first molecule\n"
				     "(just give the first system twice!)");
      }
    }

    Vec box = 0.0;
    double vtot = 0.0;
    double densit = 0.0;

    // read all topologies only to get the box length (via the mass)
    double weight=0; 
    for(unsigned int tcnt=0; tcnt<tops.size(); tcnt++) {
      InTopology it(tops[tcnt]);
      System smol(it.system());
      for(int i=0; i<smol.numMolecules();i++)
	for(int j=0; j< smol.mol(i).numAtoms();j++)
	  weight+=nsm[tcnt]*smol.mol(i).topology().atom(j).mass(); 
    }
    
    if (args.count("boxsize") > 0){

      iter=args.lower_bound("boxsize");
      {
	std::istringstream is(iter->second);
	if (!(is >> box[0]))
	  throw Arguments::Exception("could not read boxsize");
      }
      
      ++iter;
      if (iter == args.upper_bound("boxsize")){
	box[1] = box[2] = box[0];
      }
      else{
	std::istringstream is(iter->second);
	if (!(is >> box[1]))
	  throw Arguments::Exception("could not read boxsize");
	++iter;
	if (iter == args.upper_bound("boxsize"))
	  throw Arguments::Exception("could not read boxsize");
	is.clear();
	is.str(iter->second);
	if (!(is >> box[2]))
	  throw Arguments::Exception("could not read boxsize");
      }
      
      vtot = box[0] * box[1] * box[2];
      if (args["pbc"] == "t") vtot /= 2;
      densit = weight * 1.66056 / vtot;
    }
    else{
      args.check("dens",1);
      iter=args.lower_bound("dens");
      densit=atof(iter->second.c_str());

      vtot=(weight*1.66056)/densit;
      if(args["pbc"] == "t") vtot*=2;
      box=pow(vtot,1.0/3.0);
    }
    
    double thresh = 0.20;
    if (args.count("thresh") > 0) {
      istringstream ss(args["thresh"]);
      if (!(ss >> thresh))
        throw Arguments::Exception("thresh must be numeric (double).");
    }
    thresh *=thresh;
    
    bool layer = false;
    if (args.count("layer") >= 0){
      layer = true;
      std::cerr << "creating molecules in layers" << std::endl;
    }
    
    // printing the box size
    cerr << setw(20) << "Volume :" << vtot << endl
         << setw(20) << "Mass :" << weight * fac_amu2kg << endl
	 << setw(20) << "density :" << densit << endl
         << setw(20) << "cell length :" << box[0] << " x " << box[1] << " x " << box[2] << endl 
	 << setw(20) << "PBC : " << args["pbc"] << endl;
    
    // now we do the whole thing
    // new system and parse the box sizes
    System sys;
    for(int i=0;i<3;i++){
      sys.box()[i] = box[i];
    }

    // parse boundary conditions
    Boundary *pbc; 
    if(args["pbc"] == "t")
      pbc = new TruncOct(&sys);
    else 
      pbc = new RectBox(&sys);
    
    // loop over the number of topologies.
    for(unsigned int tcnt=0; tcnt<tops.size(); tcnt++) {
      
      //read topologies again
      InTopology it(tops[tcnt]);
      System smol(it.system());
      
      // read single molecule coordinates...
      InG96 ic;
      ic.open(insxs[tcnt]);
      ic >> smol;
      ic.close();
     
      // single molecule coordinates are translated to reference frame 
      //  with origin at cog
      if (tcnt != 0 || (!fixfirst))
	fit::PositionUtils::shiftToCog(&smol);

      //loop over the number of desired mol    
      for(unsigned int i=0; i < unsigned(nsm[tcnt]); ++i){
	for(int moltop = 0; moltop < smol.numMolecules(); ++moltop){

	  sys.addMolecule(smol.mol(moltop));
	  // no checks, rotation on first system... (anyway?)
	  if (tcnt == 0 && fixfirst) continue;

	  do{
	    if (layer)
	      place_random(sys, pbc, tcnt, tops.size());
	    else
	      place_random(sys, pbc);

	  } while(overlap(sys, thresh, pbc));
	  
	  cerr << (i+1) << " of " << nsm[tcnt] 
	       << " copies of molecule " << tcnt+1 
	       << " already in the box. (Total number of molecules = " 
	       << sys.numMolecules() << ")." << endl;
	}
      }
      cerr << "Box now with: " << sys.numMolecules() << " molecules" << endl;  
    }
    
    // Print the new set to cout
    OutG96S oc;
    ostringstream os;
    oc.open(cout);
    oc.writeTitle(string(os.str()));
    oc << sys;
  }
  catch (const gromos::Exception &e){
    cerr << e.what() << endl;
    return 1;
  }
  return 0;
}

/**
 * checks for overlap of all molecules os sys
 * with the last molecule of sys
 */
bool overlap(System const & sys, double threshhold, Boundary * pbc)
{
  if (sys.numMolecules() == 1) return false;
  
  const int mol2 = sys.numMolecules()-1;

  for(int mol1=0; mol1 < sys.numMolecules()-1; ++mol1){
    for(int a1=0; a1 < sys.mol(mol1).numAtoms(); ++a1){
      for(int a2=0; a2 < sys.mol(mol2).numAtoms(); ++a2){

	if ( (sys.mol(mol1).pos(a1) -
	      (pbc->nearestImage(sys.mol(mol1).pos(a1),
				 sys.mol(mol2).pos(a2),
				 sys.box()))).abs2()
	     < threshhold)
	  return true;
      }
    }
  }
  return false;
}

int place_random(System & sys, Boundary * pbc, int layer, int nlayer)
{
  const int mol = sys.numMolecules()-1;

  Vec box_mid;
  Vec rpos;
  
  switch(sys.box().boxformat()){
    case Box::box96:
      for(int d=0; d<3; ++d)
	box_mid[d] = 0.5 * sys.box()[d];
      break;
    case Box::triclinic:
      box_mid = 0.5 * (sys.box().K() + sys.box().L() + sys.box().M());
      break;
    default:
      throw gromos::Exception("ran_box", "don't know how to handle boxformat");
  }
  
  while(true){
    for(int d=0; d<3; ++d){

      const double r= double(rand()) / RAND_MAX;
      // put molecules into layers along z axis if required
      if (d == 2)
	rpos[d] = layer * (sys.box()[d] / nlayer) + r * sys.box()[d] / nlayer;
      else
	rpos[d]= r * sys.box()[d];
    }
    // correcting rpos for pbc (truncated octahedron / triclinic)
    if (rpos == pbc->nearestImage(box_mid, rpos, sys.box())) break;
  }
	  
  Vec vrot;
  for(int d=0; d<3; ++d){
    vrot[d] = double(rand()) / RAND_MAX - 0.5;
  }
  double alpha = double(rand()) / RAND_MAX * 360.0;

  Matrix m = PositionUtils::rotateAround(vrot, alpha);
  PositionUtils::rotate(sys.mol(mol), m);
  PositionUtils::translate(sys.mol(mol), rpos);

  return 0;
}
