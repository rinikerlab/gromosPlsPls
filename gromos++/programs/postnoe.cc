// noeprep.cc; 

#include <cassert>
#include <vector>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <sstream>

#include <args/Arguments.h>
#include <gio/Ginstream.h>
#include <gio/InG96.h>
#include <gcore/System.h>
#include <gio/InTopology.h>
#include <bound/Boundary.h>
#include <args/BoundaryParser.h>
#include <gmath/Distribution.h>
#include "../src/gcore/AtomTopology.h"
#include "../src/gcore/Molecule.h"
#include "../src/gcore/MoleculeTopology.h"

using namespace std;
using namespace gio;
using namespace gcore;
using namespace args;

class yaNoe
{
 public:
  yaNoe();
  yaNoe(System const &sys, int at1, int at2, double r)
    {
      //determine molecule
      int offset;
      at1--; at2--;

      for(offset=0, mol1=0; at1>=sys.mol(mol1).numAtoms()+offset; mol1++)
        offset += sys.mol(mol1).numAtoms();
      atom1=at1-offset;
      for(offset=0, mol2=0; at2>=sys.mol(mol2).numAtoms()+offset; mol2++)
        offset += sys.mol(mol2).numAtoms();
      atom2=at2-offset;
      
      r0=r;
    }
  int atom1;
  int atom2;
  int mol1;
  int mol2;
  int type;
  vector<int> collaps;
  
  double r0;
  double r_av[3];

  string h_name1;
  string h_name2;
};

void read_NOE_input(System &sys, vector<yaNoe *> &noe, string filename);
void read_NOE_output(System &sys, vector<yaNoe *> &noe, string filename);
void read_NOE_filter(System &sys, vector<yaNoe *> &noe, string filename, 
		     bool read_ref);

int main(int argc,char *argv[]){



  // known arguments...
  char *knowns[]={"topo", "noe", "noeoutput", "filter", "distance", 
		  "averaging", "ref", "minmax", "distribution"};
  int nknowns = 9;

  // Usage string

  string usage = argv[0];
  usage += "\n\t@topo      <topology>\n";
  usage += "\t@noe       <NOE specification file>\n";
  usage += "\t@noeoutput <output of noe-program>\n";
  usage += "\t@filter    <NOE filter file>\n";
  usage += "\t@[distance  <additional filter distance>]\n";
  usage += "\t@averaging <1 / 3 / 6>\n";
  usage += "\t[@ref       <noeoutput / filter>]\n";
  usage += "\t[@minmax    <min / max>\n";
  usage += "\t[@distribution <number of bins>\n";
  

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

    // where do we want to read the reference lengths from
    bool ref_from_filter=false;
    if(args.count("ref")>0){
      if(args["ref"] == "filter") ref_from_filter=true;
      else if(args["ref"] != "noeoutput"){
	throw gromos::Exception("postnoe", "Illegal value for 'ref' "
				"allowed are noeoutput or filter");
      }
    }
    
    // get a possible additional distance filter
    double cutoff=-1.0;
    bool do_distance_filter=false;
    if(args.count("distance")>0){
      cutoff = atof(args["distance"].c_str());
      do_distance_filter=true;
    }
    
    // which lengths do we want to use for the averaging
    int av_index=1;
    if(args.count("averaging")>0){
      av_index=atoi(args["averaging"].c_str())/3;
    }

    // do we want to minimise or maximise the violations upon collaps
    int minmax=1;
    if(args.count("minmax")>0){
      if(args["minmax"]=="max") minmax=-1;
      else if(args["minmax"]!="min")	
	throw gromos::Exception("postnoe", "Illegal value for 'minmax' "
				"allowed are min or max");
    }
    
    // in noe all noes will be stored.
    vector<yaNoe*> noe;

    // read the NOE input file
    read_NOE_input(sys, noe, args["noe"]);

    // Read in the NOE output
    read_NOE_output(sys, noe, args["noeoutput"]);
    
    // read in filter file
    read_NOE_filter(sys, noe, args["filter"], ref_from_filter);
    
    // =======
    // Everything read, let's start the program
    // =======

    // let's keep a list of noe's to keep
    vector<yaNoe *> keep;
    int num_distance_filtered=0;
    int num_user_filtered = 0;
    int num_collapsed =0;
    
    for(unsigned int i=0; i<noe.size(); i++){
      
      // first test: filter action should not be set to 0
      if(noe[i]->type > 0){
	
	// second test: the maybe it is filtered based on the r0
	if(!do_distance_filter || noe[i]->r0 <= cutoff){
	  
	  // third test: should it be collapsed
	  if(noe[i]->type > 1){
	    
	    //calculate the smallest noe-violation for all relevant noe's
	    double mindist=minmax*(noe[i]->r_av[av_index] - noe[i]->r0);
	    unsigned int minnoe=i;
	    for(int j=0; j<noe[i]->type-1; j++){
	      
	      if(noe[noe[i]->collaps[j]]->type!=0){
		
		double v=minmax*(noe[noe[i]->collaps[j]]->r_av[av_index] - 
				 noe[noe[i]->collaps[j]]->r0);
		if(v<mindist){
		  mindist=v;
		  minnoe=noe[i]->collaps[j];
		}
		// flag it so that it will not be considered anymore
		noe[noe[i]->collaps[j]]->type=-1;
	      }
	    }
	    
	    keep.push_back(noe[minnoe]);
	    num_collapsed+=noe[i]->type-1;
	  }
	  else{
	    keep.push_back(noe[i]);
	  }
	}
	else num_distance_filtered++;
      }
      else if(noe[i]->type==0) num_user_filtered++;
      
    }

    // write out and average the noe's we have left and make a distribution
    int dist_bin=0;
    if(args.count("distribution")>0){
      dist_bin=atoi(args["distribution"].c_str());
    }
    

    // cout << "we have now " << keep.size() << " NOE's left" << endl;
    // get the averages and minimum and maximum violation
    double s_r0=0, s_viol=0, ss_viol=0, min_viol=0, max_viol=0;
    double viol;
    int num_viol=0;
    
    for(unsigned int i=0; i< keep.size(); i++){
      s_r0 += keep[i]->r0;
      viol = keep[i]->r_av[av_index] - keep[i]->r0;
      if(viol>0){
	num_viol++;
	s_viol+= viol;
	ss_viol += viol * viol;
      }
      if(viol < min_viol) min_viol=viol;
      if(viol > max_viol) max_viol=viol;
    }
    int bla=100;
    if(dist_bin) bla=dist_bin;
    // add roughly one more bin, so that you also get the highest itself
    // (a Distribution goes from min <= value < max)
    max_viol += (max_viol-min_viol) / bla;
    
    gmath::Distribution dist(min_viol, max_viol, bla);

    cout << "TITLE\n"
	 << "Postprocessing NOE data using:\n"
	 << "\tNOE input  : " << args["noe"] << endl
	 << "\tNOE output : " << args["noeoutput"] << endl
	 << "\tNOE filter : " << args["filter"] << endl;

    cout << "\nUpper bounds have been taken from ";
    if(ref_from_filter) cout << "NOE filter file\n";
    else cout << "NOE input file\n";
    
    cout << "\n" << setw(5) << noe.size() << " initial NOE's to analyse\n";
    
    if(num_user_filtered)
      cout << "\n" << setw(5) << num_user_filtered 
	   << " NOE's have been removed "
	   << "by direct filtering according to user input\n";
    if(num_distance_filtered)
      cout << "\n" << setw(5) << num_distance_filtered 
	   << " NOE's have been removed "
	   << "according to distance criterion (r0 > " << cutoff << ")\n";
    if(num_collapsed)
      cout << "\n" << setw(5) << num_collapsed 
	   << " NOE's have been removed after "
	   << "assignment of stereospecific NOE's\n"
	   << "            assignment was based on " << args["minmax"] 
	   << "imum violations\n";
    if( num_collapsed+num_user_filtered+num_distance_filtered)
      cout << "\n" << setw(5) << keep.size() << " NOE's left over\n";
    
    cout << "\nEND\n";
    
    int averaging=3;
    if(av_index==0) averaging=1;
    else if(av_index==1) averaging=3;
    else if(av_index==2) averaging=6;

    cout << "AVERAGE NOE VIOLATIONS"
	 << "\n#\n# Average NOE distances calculated as <r^-" << averaging
	 << ">^-" << averaging
	 << "\n#\n";
    
    cout << "#   "
	 << setw(4) << "mol"
	 << setw(10) << "residue"
	 << setw(10) << "atom"
	 << setw(5) << "orig"
	 << setw(8) << "mol"
	 << setw(10) << "residue"
	 << setw(10) << "atom  "
	 << setw(5) << "orig"
	 << setw(12) << "r0"
	 << setw(8) << "r_av"
	 << setw(8) << "viol" 
	 << endl;
    
    for(unsigned int i=0; i< keep.size(); i++){
      viol = keep[i]->r_av[av_index] - keep[i]->r0;
      
      cout << setw(4) << i+1 
	   << setw(4) << noe[i]->mol1+1
	   << setw(5) << sys.mol(noe[i]->mol1).topology().resNum(noe[i]->atom1)+1
	   << setw(5) << left << sys.mol(noe[i]->mol1).topology().resName(sys.mol(noe[i]->mol1).topology().resNum(noe[i]->atom1))
	   << setw(5) << right << noe[i]->atom1+1
	   << setw(5) << sys.mol(noe[i]->mol1).topology().atom(noe[i]->atom1).name()
	   << setw(5) << noe[i]->h_name1
	   << setw(8) << noe[i]->mol2+1
	   << setw(5) << sys.mol(noe[i]->mol2).topology().resNum(noe[i]->atom2)+1
	   << setw(5) << left << sys.mol(noe[i]->mol2).topology().resName(sys.mol(noe[i]->mol2).topology().resNum(noe[i]->atom2))
	   << setw(5) << right << noe[i]->atom2+1
	   << setw(5) << sys.mol(noe[i]->mol2).topology().atom(noe[i]->atom2).name()
	   << setw(5) << noe[i]->h_name2
	   << setw(12) << noe[i]->r0
	   << setw(8) << noe[i]->r_av[av_index]
	   << setw(8) << viol 
	   << endl;
      if(dist_bin) dist.add(viol);
    }
    cout << "END\n";
    cout << "VIOLATION AVERAGES\n";
    cout << setw(5) << keep.size() << " NOE's analysed\n"
	 << "      Average r0            " << s_r0 / keep.size() << "\n\n"
	 << setw(5) << num_viol << " violations\n";
    if(num_viol)
      cout << "      Average of violations " << s_viol/keep.size()
	  << "\n      RMS violations        " 
	   << sqrt((ss_viol - s_viol*s_viol/keep.size())/keep.size()) << endl;
    cout << "END\n";
    
    if(dist_bin){
      cout << "VIOLATION DISTRIBUTION\n";
      cout << "# "
	   << setw(6) << "viol"
	   << setw(13) << "count"
	   << endl;
      
      dist.write(cout);
      cout << "END" << endl;;
    }
    
    
    
    
	
  
         
  }
  
  catch(gromos::Exception e){
    cerr << e.what() << endl;
    exit(1);
  }
  return 0;
}

void read_NOE_input(System &sys, vector<yaNoe *> &noe, string filename)
{
  // Read in and create the NOE list from the noe input
  Ginstream nf(filename);
  vector<string> buffer;
  nf.getblock(buffer);
  
  if(buffer[0]!="DISRESSPEC")
    throw gromos::Exception("main",
			    "NOE file does not contain an DISRESSPEC block!");
  if(buffer[buffer.size()-1].find("END")!=0)
    throw gromos::Exception("postnoe","NOE file " + nf.name() +
			    " is corrupted. No END in DISRESSPEC"
			    " block. Got\n"
			    + buffer[buffer.size()-1]);
  int at1,at2,idum;
  double r;
  string sdum;
  istringstream is;
  
  // we don't care about the distances here, so start reading at the 2nd
  for(unsigned int j=2; j< buffer.size()-1; j++){
    is.clear();
    is.str(buffer[j]);
    is >> at1 >> idum >> idum >> idum >> idum
       >> at2 >> idum >> idum >> idum >> idum
       >> r;
    
    noe.push_back(new yaNoe(sys, at1, at2, r));
  }
  
  nf.close();
  
}

void read_NOE_output(System &sys, vector<yaNoe *> &noe, string filename)
{
  Ginstream nf(filename);
  vector<string> buffer;
  istringstream is;

  int idum;
  string sdum;
  double r;
  
  nf.getblock(buffer);
  if(buffer[0]!="AVERAGE NOE")
    throw gromos::Exception("postnoe", "No AVERAGE NOE block in file "
			    + filename);
  if(buffer.size()-2!=noe.size()){
    throw gromos::Exception("postnoe","NOE input file and AVERAGE NOE "
			    "block do not have the same number of NOE's\n");
  }
  if(buffer[buffer.size()-1].find("END")!=0)
      throw gromos::Exception("postnoe","NOE output file " + nf.name() +
			      " is corrupted. No END in AVERAGE"
			      " block. Got\n"
			      + buffer[buffer.size()-1]);

  for(unsigned int i=0; i<noe.size(); i++){
    is.clear();
    is.str(buffer[i+1]);
    is >> idum 
       >> noe[i]->r_av[0] 
       >> noe[i]->r_av[1] 
       >> noe[i]->r_av[2];
  }
  buffer.clear();
  nf.getblock(buffer);
  if(buffer[0]!="NOE VIOLATIONS")
    throw gromos::Exception("postnoe", "No NOE VIOLATIONS block in file "
			    + filename);
  if(buffer.size()-2!=noe.size()){
    throw gromos::Exception("postnoe","NOE input file and NOE VIOLATIONS "
			    "block do not have the same number of NOE's\n");
  }
  if(buffer[buffer.size()-1].find("END")!=0)
      throw gromos::Exception("postnoe", "NOE output file " + nf.name() +
			      " is corrupted. No END in VIOLATIONS"
			      " block. Got\n"
			      + buffer[buffer.size()-1]);
  for(unsigned int i=0; i<noe.size(); i++){
    is.clear();
    is.str(buffer[i+1]);
    is >> sdum >> r;
    if(r!=noe[i]->r0){
      throw gromos::Exception("postnoe", "NOE distance has changed from NOE"
			      " input to NOE output file in NOE "+sdum);
    }
  }
  nf.close();
    
}

void read_NOE_filter(System &sys, vector<yaNoe *> &noe, string filename, 
		     bool read_ref)
{
  Ginstream nf(filename);
  vector<string> buffer;
  istringstream is;

  nf.getblock(buffer);
  if(buffer[0]!="NOEFILTER"){
    throw gromos::Exception("postnoe", "No NOEFILTER block in file "
			    + filename);
  }
  if(buffer.size()-2!=noe.size()){
    throw gromos::Exception("postnoe", "NOE input file and NOE filter file "
			    "do not have the same number of NOE's");
  }
  if(buffer[buffer.size()-1].find("END")!=0)
    throw gromos::Exception("postnoe", "Filter file " + nf.name() +
			    " is corrupted. No END in NOEFILTER"
			    " block. Got\n"
			    + buffer[buffer.size()-1]);
  
  for(unsigned int i=0; i<noe.size(); i++){
    int m1, r1, m2, r2, idum;
    string rn1, atn1, rn2, atn2, sdum;
    double r_filt;
    
    is.clear();
    is.str(buffer[i+1]);
    is >> sdum 
       >> m1 >> r1 >> rn1 >> atn1 >> noe[i]->h_name1
       >> m2 >> r2 >> rn2 >> atn2 >> noe[i]->h_name2
       >> r_filt >> noe[i]->type;
    for(int j=0; j<noe[i]->type-1; j++){
      is >> idum;
      noe[i]->collaps.push_back(idum-1);
    }
    // check input
    if(m1 != noe[i]->mol1 +1){
      throw gromos::Exception("postnoe", "NOE filter file does not match "
			      "NOE input file and topology (mol1)"
			      +buffer[i+1]);
    }
    if(m2 != noe[i]->mol2 +1){
      throw gromos::Exception("postnoe", "NOE filter file does not match "
			      "NOE input file and topology (mol2)"
			      +buffer[i+1]);
    }
    //cout << r1 << " " << noe[i]->atom1 << " " << sys.mol(noe[i]->mol1).topology().resNum(noe[i]->atom1) << endl;
    
    if(r1 != sys.mol(noe[i]->mol1).topology().resNum(noe[i]->atom1)+1){
      throw gromos::Exception("postnoe", "NOE filter file does not match "
			      "NOE input file and topology (resNum1)"
			      +buffer[i+1]);
    }
    if(r2 != sys.mol(noe[i]->mol2).topology().resNum(noe[i]->atom2)+1){
      throw gromos::Exception("postnoe", "NOE filter file does not match "
			      "NOE input file and topology (resNum2)"
			      +buffer[i+1]);
    }	
    if(rn1 != sys.mol(noe[i]->mol1).topology().
       resName(sys.mol(noe[i]->mol1).topology().resNum(noe[i]->atom1))){
      throw gromos::Exception("postnoe", "NOE filter file does not match "
			      "NOE input file and topology (resName1)"
			      +buffer[i+1]);
    }
    if(rn2 != sys.mol(noe[i]->mol2).topology().
       resName(sys.mol(noe[i]->mol2).topology().resNum(noe[i]->atom2))){
      throw gromos::Exception("postnoe", "NOE filter file does not match "
			      "NOE input file and topology (resName2)"
			      +buffer[i+1]);
    }
    if(atn1 != sys.mol(noe[i]->mol1).topology().atom(noe[i]->atom1).name()){
      throw gromos::Exception("postnoe", "NOE filter file does not match "
			      "NOE input file and topology (Atom1)"
			      +buffer[i+1]);
    }
    if(atn2 != sys.mol(noe[i]->mol2).topology().atom(noe[i]->atom2).name()){
      throw gromos::Exception("postnoe", "NOE filter file does not match "
			      "NOE input file and topology (Atom2)"
			      +buffer[i+1]);
    }
    if(read_ref)
      noe[i]->r0=r_filt;
  }
  nf.close();
  
}

