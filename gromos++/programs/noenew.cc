// noe.cc

#include <cassert>
#include <args/Arguments.h>
#include <gio/Ginstream.h>
#include <gio/InG96.h>
#include <gcore/System.h>
#include <gio/InTopology.h>
#include <gio/StringTokenizer.h>
#include <bound/Boundary.h>
#include <args/BoundaryParser.h>
#include <args/GatherParser.h>
#include <utils/Noe.h>
#include <gmath/Vec.h>

#include <vector>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <cmath>

using namespace gcore;
using namespace args;
using namespace gio;
using namespace bound;
using namespace utils;
using namespace std;


int main(int argc,char *argv[]){


  // Usage string

  string usage = argv[0];
  usage += "\n\t@topo <topology>\n";
  usage += "\t@pbc <boundary type> [ <connectivity atoms> ]\n";
  usage += "\t@traj <trajectory files>\n";
  usage += "\t@noe <NOE specification file, PROADR format>\n"; 



  // defining all sorts of constants


  // known arguments...
  char *knowns[]={"topo", "noe", "pbc", "traj" };
  int nknowns = 4;
    
  // prepare cout for formatted output
  cout.setf(ios::right, ios::adjustfield);
  cout.setf(ios::fixed, ios::floatfield);
  cout.precision(3);
    
  try{

    // Getting arguments and checking if everything is known.
    Arguments args(argc,argv,nknowns,knowns,usage);

    // read topology
    InTopology it(args["topo"]);
    System sys(it.system());

    // parse boundary conditions
    Boundary *pbc = BoundaryParser::boundary(sys, args);
    //parse gather method
    Boundary::MemPtr gathmethod = args::GatherParser::parse(args);
    // Read in and create the NOE list
    Ginstream nf(args["noe"]);
    vector<string> buffer;
    nf.getblock(buffer);
    
    if(buffer[0]!="DISRESSPEC")
      throw gromos::Exception("main","NOE file does not contain an DISRESSPEC block!");

    // in noe all noes will be stored.
    vector<Noe*> noe;

    string line;
    StringTokenizer tok(buffer[1]);
    vector<string> dishdisc = tok.tokenize();
    double dish = atof(dishdisc[0].c_str());
    double disc = atof(dishdisc[1].c_str());
    for(unsigned int j=2; j< buffer.size()-1; j++){      
      noe.push_back(new Noe(sys, buffer[j], dish, disc));
    }
    
    nf.close();

    
    // vectors to contain the r**-3 and r**-6 averages
    vector<vector<double> > av, av3, av6;
    
    // initialisation of av3 and av6
    av.resize(noe.size());
    av3.resize(noe.size());
    av6.resize(noe.size());
    
    
    for(unsigned int i=0; i<noe.size(); ++i){
      av[i].resize(noe[i]->numDistances());
      av3[i].resize(noe[i]->numDistances());
      av6[i].resize(noe[i]->numDistances());
    }

            
    // define input coordinate
    InG96 ic;
    
    int numFrames=0;
    
    
    // loop over all trajectories
    for(Arguments::const_iterator 
	  iter=args.lower_bound("traj"),
	  to=args.upper_bound("traj");
	iter!=to; ++iter){
      
      // open file
      ic.open((iter->second).c_str());
      
      // loop over single trajectory
      while(!ic.eof()){
	numFrames++;
	ic >> sys;
	(*pbc.*gathmethod)();
	
	
	
	// loop over noes
	for(int i=0; i < int(noe.size()); ++i){

	  for(int ii=0; ii < noe[i]->numDistances(); ++ii){
	    // calculate distance and averages...
	    double distance=noe[i]->distance(ii);
	    av[i][ii]+=distance;
	    av3[i][ii]+=pow(distance,-3.0);
	    av6[i][ii]+=pow(distance,-6.0);
	  }
	}
      }
      ic.close();
    }

    // normalise the averages
    for(int i=0; i < int(noe.size());  ++i)
      for(int ii=0; ii < noe[i]->numDistances(); ++ii){
	av[i][ii]/=numFrames;
	av3[i][ii]=pow(av3[i][ii]/numFrames,-1.0/3.0);
	av6[i][ii]=pow(av6[i][ii]/numFrames,-1.0/6.0);
      }
    

    // output the averages
    cout << "AVERAGE NOE\n";
    for(int i=0, nr=1; i < int(noe.size()); ++i)
      for(int j=0; j < noe[i]->numDistances(); ++j, ++nr)
	cout << "# " << setw(4) << nr << " " << noe[i]->info(j) << endl;
    
    cout <<'#' << ' ' 
	 << setw(4) << "Nr."
	 << setw(10) << "<r>"
         << setw(20) << "<r**-3>**-1/3"
         << setw(20) << "<r**-6>**-1/6" << endl;

    for(unsigned int i=0, nr=1, num=noe.size(); i<num; ++i)
      for(unsigned int ii=0, nnum=noe[i]->numDistances(); ii<nnum;++ii, ++nr)
	cout << setw(6) << nr
	     << setw(10) << av[i][ii]
	     << setw(15) << av3[i][ii]
	     << setw(20) << av6[i][ii]
	     << endl;

    
    // Now treat the violations
    
    // order the distances first
    vector<vector<int> > order(noe.size());
    for(unsigned int i=0, num=noe.size(); i<num; ++i){
      order[i].push_back(0);
      for(unsigned int ii=1, nnum=noe[i]->numDistances(); ii<nnum;++ii)
	for(vector<int>::iterator
	      it=order[i].begin(),
	      to=order[i].end()+1;
	    it!=to;++it)
	  if(it==to-1)order[i].insert(it,ii);
	  else if(av3[i][ii]<av3[i][*it]){
	    order[i].insert(it, ii);
	    break;
	  }
    }

    double cdaver=0.0, daver=0.0, avresvio=0.0, avresvio3=0.0, avresvio6=0.0; int c=0; 
    // now output the NOE violations
    cout << "END\nNOE VIOLATIONS\n";
    for(unsigned int i=0, nr=1, num=noe.size(); i<num; ++i)
      for(unsigned int ii=0, nnum=noe[i]->numReferences(); ii<nnum;++ii, ++nr)
	cout << "# " << setw(4) << nr << " " << noe[i]->info(ii) << endl;
    cout << "#\n";
    cout << "# d=experimental distance, cd=experimental distance plus correction\n";

    cout <<'#' << ' ' 
	 << setw(4) << "Nr."
	 << setw(10) << "d"
	 << setw(10) << "cd"
	 << setw(10) << "<r> - cd"
         << setw(20) << "<r**-3>**-1/3 - cd"
         << setw(20) << "<r**-6>**-1/6 - cd" << endl;
    
    for(unsigned int i=0, nr=1, num=noe.size(); i<num; ++i)
      for(unsigned int ii=0, nnum=noe[i]->numReferences(); ii<nnum;++ii, ++nr){
	double cd;
	
	  cd=noe[i]->reference(ii);
	
	//averaging stuff
        daver+=	noe[i]->reference(ii);
        cdaver+=cd;
        c+=1;
        if (av[i][order[i][ii]] - cd > 0.0){
          avresvio+=av[i][order[i][ii]] - cd;
	}
        if (av3[i][order[i][ii]] - cd > 0.0){
          avresvio3+=av3[i][order[i][ii]] - cd;
	}
        if (av6[i][order[i][ii]] - cd > 0.0){
          avresvio6+=av6[i][order[i][ii]] - cd;
	}

	//the real printout
	cout <<	setw(6) << nr 
	     << setw(10) <<noe[i]->reference(ii)
	     << setw(10) << cd 
	     << setw(10) << av[i][order[i][ii]] - cd
	     << setw(15) << av3[i][order[i][ii]] - cd 
	     << setw(20) << av6[i][order[i][ii]] - cd << endl;
      }

    cout << "END\n";
    cout << "AVERAGE d: " << cdaver/c << endl;
    cout << "AVERAGE cd: " << cdaver/c << endl;
    cout << "AVERAGE RESTRAINT VIOLATION (<av-cd>): " << avresvio/c << endl;
    cout << "AVERAGE RESTRAINT VIOLATION (<av3-cd>): " << avresvio3/c << endl;
    cout << "AVERAGE RESTRAINT VIOLATION (<av6-cd>): " << avresvio6/c << endl;
  }
  catch(gromos::Exception e){
    cerr << e.what() << endl;
    exit(1);
  }
  return 0;
}
