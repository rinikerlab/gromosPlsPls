//distributions dist

// written by Mika, modified by Markus

#include <cassert>

#include "../src/args/Arguments.h"
#include "../src/bound/TruncOct.h"
#include "../src/bound/Vacuum.h"
#include "../src/bound/RectBox.h"
#include "../src/args/BoundaryParser.h"
#include "../src/args/GatherParser.h"
#include "../src/gio/InG96.h"
#include "../src/gcore/System.h"
#include "../src/gcore/Molecule.h"
#include "../src/gio/InTopology.h"
#include "../src/bound/Boundary.h"
#include "../src/gmath/Vec.h"
#include "../src/gmath/Distribution.h"
#include "../src/utils/AtomSpecifier.h"
#include "../src/utils/PropertyContainer.h"
#include <vector>
#include <iomanip>
#include <math.h>
#include <iostream>

using namespace std;
using namespace gcore;
using namespace gio;
using namespace bound;
using namespace args;
using namespace utils;

int main(int argc, char **argv){

  char *knowns[] = {"topo", "pbc", "prop", "dist", "traj", "norm", "solv"};
  int nknowns = 7;

  string usage = argv[0];
  usage += "\n\t@topo   <topology>\n";
  usage += "\t@pbc    <boundary type>\n";
  usage += "\t@dist   <lower and upper boundary and number of steps>\n";
  usage += "\t@prop   <propertyspecifier>\n";
  usage += "\t@traj   <trajectory files>\n";
  usage += "\t[@norm   normalize the distribution\n";
  usage += "\t[@solv   read in solvent as well\n";
 
try{
  Arguments args(argc, argv, nknowns, knowns, usage);

  //   get distribution parameters
  //   maybe change to a statistics class, get distribution from there...
  //   CHRIS???
  double begin=0, end=0;
  int nsteps=0; 
  {
    Arguments::const_iterator iter=args.lower_bound("dist");
    if(iter!=args.upper_bound("dist")){
      begin=atof(iter->second.c_str());
      ++iter;
    }
    if(iter!=args.upper_bound("dist")){
      end=atof(iter->second.c_str());
      ++iter;
    }
    if(iter!=args.upper_bound("dist")){
      nsteps=atoi(iter->second.c_str());
    }     
  }
  
  //  read topology
  args.check("topo",1);
  InTopology it(args["topo"]);
  System sys(it.system());

  // get properties into PropertySpecifier
  // these are the standard properties we want to calculate
  // at every timestep
  // these will be added to the standard PropertyContainer distribution
  PropertyContainer props(sys);
  {
    Arguments::const_iterator iter=args.lower_bound("prop");
    Arguments::const_iterator to=args.upper_bound("prop");
    for(; iter!=to; iter++)
      {
	string spec=iter->second.c_str();
	props.addSpecifier(spec);
      }    
  }

  // set up distribution arrays
  gmath::Distribution dist(begin, end, nsteps);
  props.addDistribution(dist);

  // Parse boundary conditions and get gather method
  Boundary *pbc = BoundaryParser::boundary(sys, args); 
  Boundary::MemPtr gathmethod = args::GatherParser::parse(args);

  // define input coordinate
  InG96 ic;

  bool normalize = false;
  if (args.count("norm") != -1)
    normalize = true;

  bool solvent = false;
  if (args.count("solv") != -1)
		 solvent = true;

  // the "real" average (distribution takes the middle of the bins)
  // CHRIS: this would also be better with a statistics class
  double average = 0.0;
  int steps = 0;
  
  // loop over all trajectories
  for(Arguments::const_iterator 
        iter=args.lower_bound("traj"),
        to=args.upper_bound("traj");
      iter!=to; ++iter){

    // open file
    ic.open((iter->second).c_str());
    if (solvent) ic.select("ALL");
    
      // loop over single trajectory
    while(!ic.eof()){
      ic >> sys;
      (*pbc.*gathmethod)();
      props.calc();
      cout << props.checkBounds();
      double av, rmsd, zrmsd, lb, ub;
      int lp, up;
      
      props.averageOverProperties(av, rmsd, zrmsd, lb, ub, lp, up);
      average += av;
      ++steps;
    }
  }
  ic.close();
  // this already has been the main program!!!

  // print out the distribution, calculate the average and rmsd
  cout << "#" << endl;  
  cout << "# number of values calculated: "   
       << props.getDistribution().nVal() << endl;
  cout << "# average value:               "   
       << props.getDistribution().ave() << endl;
  cout << "# RMSD (from distribution):    "   
       << props.getDistribution().rmsd() << endl;

  cout << "# real average\t\t" << average / steps << endl;

  //cout << "# time\t\t" <<  props.toTitle() << endl;

  if (normalize)
    props.getDistribution().write_normalized(cout);
  else
    props.getDistribution().write(cout);

  }
  catch (const gromos::Exception &e){
    cerr << e.what() << endl;
    exit(1);
  }
  return 0;
}

