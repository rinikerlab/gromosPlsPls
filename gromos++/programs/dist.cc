//distributions dist

#include "../src/args/Arguments.h"
#include "../src/bound/TruncOct.h"
#include "../src/bound/Vacuum.h"
#include "../src/bound/RectBox.h"
#include "../src/args/BoundaryParser.h"
#include "../src/fit/Reference.h"
#include "../src/gio/InG96.h"
#include "../src/gcore/System.h"
#include "../src/gcore/Molecule.h"
#include "../src/gio/InTopology.h"
#include "../src/bound/Boundary.h"
#include "../src/fit/PositionUtils.h"
#include "../src/gmath/Vec.h"
#include "../src/gmath/Distribution.h"
#include "../src/utils/AtomSpecifier.h"
#include <vector>
#include <iomanip>
#include <math.h>
#include <iostream>

using namespace fit;
using namespace gcore;
using namespace gio;
using namespace bound;
using namespace args;
using namespace utils;

int main(int argc, char **argv){

  char *knowns[] = {"topo", "pbc", "prop", "atoms", "dist", "traj"};
  int nknowns = 6;

  string usage = argv[0];
  usage += "\n\t@topo   <topology>\n";
  usage += "\t@pbc    <boundary type>\n";
  usage += "\t@dist   <lower and upper boundary and number of steps>\n";
  usage += "\t@prop   <property to calculate: (d) distance (a) bondangle (t) torsional angle>\n";
  usage += "\t@atoms  <atomspecifier>\n";
  usage += "\t@traj   <trajectory files>\n";
  
 
try{
  Arguments args(argc, argv, nknowns, knowns, usage);

  //   get distribution parameters
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

  // get atoms into AtomSpecifier
  AtomSpecifier atoms(sys);
  {
    Arguments::const_iterator iter=args.lower_bound("atoms");
    Arguments::const_iterator to=args.upper_bound("atoms");
    for(; iter!=to; iter++)
      {
	string spec=iter->second.c_str();
	atoms.addSpecifier(spec);
      }    
  }

  enum prop_type { DISTANCE=2, BONDANGLE=3, DIHEDRALANGLE=4 } prop;
  // Parse property type
  {
    args.check("prop",1);
    char b=args["prop"].c_str()[0];
    switch(b){
      case 'd':
	prop=DISTANCE;
	break;
      case 'a':
	prop=BONDANGLE;
	break;
      case 't':
	prop=DIHEDRALANGLE;
	break;
      default:
	throw gromos::Exception("Property", " unknown. Known: (d)istance, (b)ondangle, d(i)hedralangle");
    }
  }
  
  // consistency check
  {
    switch(prop){
      case DISTANCE:
	if (atoms.size()%2) throw gromos::Exception("Atoms", " must be even number to calculate distances");
	break;
      case BONDANGLE:
	if (atoms.size()%3) throw gromos::Exception("Atoms:", " need pairs of three atoms to calculate bondangles");
	break;
      case DIHEDRALANGLE:
	if (atoms.size()%4) throw gromos::Exception("Atoms:", " need pairs of four atoms to calculate dihedral angles");
	break;
    }
  }
  
  // Parse boundary conditions
  Boundary *pbc;
  try{
    char b=args["pbc"].c_str()[0];
    switch(b){
      case 't':
        pbc=new TruncOct(&sys);
        break;
      case 'v':
        pbc=new Vacuum(&sys);
        break;
      case 'r':
        pbc=new RectBox(&sys);
        break;
      default:
        throw gromos::Exception("Boundary", args["pbc"] + 
				" unknown. Known boundaries are t, r and v");
	
    }
  }
  catch(Arguments::Exception &e){
    pbc = new Vacuum(&sys);
  }

  // define input coordinate
  InG96 ic;

  // set up distribution arrays
  gmath::Distribution dist(begin, end, nsteps);

  // set pi
  const double pi = 3.1415926535898;
  
  // loop over all trajectories
  for(Arguments::const_iterator 
        iter=args.lower_bound("traj"),
        to=args.upper_bound("traj");
      iter!=to; ++iter){

    // open file
    ic.open((iter->second).c_str());
       // ic.select("ALL");
    
      // loop over single trajectory
    while(!ic.eof()){
      ic >> sys;
      pbc->gather();
      
      for(int i=0; i<atoms.size(); i+=prop)
	{

	  double val = 0;

	  if (prop == DISTANCE) 
	    {  
	      Vec tmp = (sys.mol(atoms.mol(i)).pos(atoms.atom(i))
			 -sys.mol(atoms.mol(i+1)).pos(atoms.atom(i+1))); 
	      val = tmp.abs();
	    }
	  else if (prop == BONDANGLE) 
	    {
	      Vec tmpA = (sys.mol(atoms.mol(i)).pos(atoms.atom(i))
			  -sys.mol(atoms.mol(i+1)).pos(atoms.atom(i+1))); 
	      Vec tmpB = (sys.mol(atoms.mol(i+2)).pos(atoms.atom(i+2))
			  -sys.mol(atoms.mol(i+1)).pos(atoms.atom(i+1)));

	      val = acos((tmpA.dot(tmpB))/(tmpA.abs()*tmpB.abs()))*180/pi;
	    }          
	  else if (prop == DIHEDRALANGLE) {
	    Vec tmpA = (sys.mol(atoms.mol(i)).pos(atoms.atom(i))
			-sys.mol(atoms.mol(i+1)).pos(atoms.atom(i+1)));
	    Vec tmpB = (sys.mol(atoms.mol(i+3)).pos(atoms.atom(i+3))
			-sys.mol(atoms.mol(i+2)).pos(atoms.atom(i+2)));
	    Vec tmpC = (sys.mol(atoms.mol(i+2)).pos(atoms.atom(i+2))
			-sys.mol(atoms.mol(i+1)).pos(atoms.atom(i+1)));
	    
	    Vec p1 = tmpA.cross(tmpC);
	    Vec p2 = tmpB.cross(tmpC);

	    double cosphi = ((p1.dot(p2))/(p1.abs()*p2.abs()));
 
	    val = acos(cosphi)*180/pi;
 
	    Vec p3 = p1.cross(p2);
	    if (p3.dot(tmpC)<0)
	      val = 360 - val;
	  }
      
	  // count this value in the distribution array
	  dist.add(val);
	}
    }
  }
  ic.close();
  // print out the distribution, calculate the average and rmsd
  cout << "#" << endl;
  switch(prop)
    {
    case DISTANCE:
      cout << "# Distance distribution\n";
      break;
    case BONDANGLE:
      cout << "# Bond angle distribution\n";
      break;
    case DIHEDRALANGLE:
      cout << "# Dihedral angle distribution\n";
      break;
    }
  
  cout << "#";

  for(int i=0; i<atoms.size(); i+=prop)
    {
      cout << "\t" << atoms.mol(i) << ":" << atoms.atom(i);
      for(int j=1; j<prop; j++)
	cout << "-" << atoms.atom(i+j);
      if (i>10) {
	cout << "...";
	break;
      }
    }
  cout << endl;
  
  cout << "# number of values calculated: "   << dist.nVal() << endl;
  cout << "# average value:               "   << dist.ave() << endl;
  cout << "# RMSD (from distribution):    "   << dist.rmsd() << endl;
  dist.write(cout);
    
  }
  catch (const gromos::Exception &e){
    cerr << e.what() << endl;
    exit(1);
  }
  return 0;
}






