#include <cassert>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include "../src/args/Arguments.h"
#include "../src/gio/InTopology.h"
#include "../src/gio/InParameter.h"
#include "../src/gio/InBuildingBlock.h"
#include "../src/gio/OutTopology.h"
#include "../src/gcore/GromosForceField.h"
#include "../src/gcore/System.h"
#include "../src/gcore/Molecule.h"
#include "../src/gcore/MoleculeTopology.h"
#include "../src/gcore/Solvent.h"
#include "../src/gcore/SolventTopology.h"
#include "../src/gcore/BuildingBlock.h"
#include "../src/gcore/BbSolute.h"
#include "../src/gcore/SolventTopology.h"
#include "../src/gcore/AtomTopology.h"
#include "../src/gcore/Exclusion.h"
#include "../src/gcore/MassType.h"
#include "../src/gcore/Bond.h"
#include "../src/gcore/Angle.h"
#include "../src/gcore/Improper.h"
#include "../src/gcore/Dihedral.h"
#include "../src/gcore/LinearTopology.h"

// for sscanf
#include <stdio.h>

using namespace std;
using namespace gcore;
using namespace gio;
using namespace args;

#include "../src/utils/maketop.h"

int main(int argc, char *argv[]){

  char *knowns[] = {"build", "param", "seq", "solv", "cys"};
  int nknowns = 5;
  
  string usage = argv[0];
  usage += "\n\t@build <building block file>\n";
  usage += "\t@param <gromos parameter file>\n";
  usage += "\t@seq   <sequence>\n";
  usage += "\t@solv  <solvent>\n";
  usage += "\t@cys   <cys1>-<cys2>\n";
  
  
  
  try{
    Arguments args(argc, argv, nknowns, knowns, usage);
    
    // read in the force field
    InParameter ip(args["param"]);
    GromosForceField gff(ip.forceField());
    InBuildingBlock ibb(args["build"]);
    BuildingBlock mtb(ibb.building());

    // parse the input for disulfide bridges    
    vector<int> cys1, cys2, csa1, csa2;
    
    for(Arguments::const_iterator iter=args.lower_bound("cys"),
	  to=args.upper_bound("cys"); iter!=to; ++iter){
      string s=iter->second;
      
      int a;
      std::string::size_type iterator;
      iterator = s.find('-');
      if (iterator == std::string::npos)
        throw gromos::Exception("maketop", "Bad cysteine specification\n");
      if (sscanf((s.substr(0,iterator)).c_str(),"%d", &a) != 1)
        throw gromos::Exception("maketop", 
	       "Bad first cysteine specification: "+s+"\n");
      cys1.push_back(--a);
      if (sscanf((s.substr(iterator+1,s.length())).c_str(),"%d", &a) != 1)
        throw gromos::Exception("maketop", 
		"Bad second cysteine specification: "+s+"\n");
      cys2.push_back(--a);
    }

    //some variables and lists to store data temporarily
    int index=0;
    //status=0: normal linking
    //status=1: current is a beginning
    //status=2: current is first after beginning
    //status=3: current is an end
    int status=0;
    int repforward=0;
    // firstAtom is the first atom of the current molecule as determined
    // from a starting end-group
    int firstAtom=0;
    
    gcore::LinearTopology lt;
    int resnum=0;
    int cyclic=0;
    
    
    // loop over the sequence
    for(Arguments::const_iterator iter=args.lower_bound("seq"),
	  to=args.upper_bound("seq"); iter!=to; ++iter){

      if(iter->second == "cyclic"){
	if(lt.atoms().size())
	  throw(gromos::Exception("maketop", "Maketop can only cyclize one complete molecule. The keyword cyclic should be the first in the sequence"));
        prepareCyclization(lt);
        iter++;
	status = 1;
        repforward = 0;
	cyclic=1;
      }
      
      index = mtb.findBb(iter->second);
      
      if(index==0) throw gromos::Exception("maketop", 
		  "Cannot find building block for "
		 +iter->second+" in "+args["build"]);
      
      //determine index and status
      if(index<0) {
	index=-1-index; 
        if(mtb.be(index).rep()<0) status=3;
	else status = 1;
      }
      else {
	index=index-1; 
	if (status==1) status =2;
	else status = 0;
      }
      
      // depending on the status add the correct building block to the
      // linearTopology
      switch(status){
      case 0:
        addSolute(lt, mtb.bb(index), resnum, iter->second, 0, firstAtom);
        resnum++;
	break;
      case 1:
        repforward = addBegin(lt, mtb.be(index), resnum);
	firstAtom = lt.atoms().size() - mtb.be(index).numAtoms();
        addCovEnd(lt, mtb.be(index), firstAtom);
	break;
      case 2:
        addSolute(lt, mtb.bb(index), resnum, iter->second, 
		  repforward, firstAtom);
	// a call to remove atoms, because we might have some negative iac's
	// from the beginning buildingblock.
        lt.removeAtoms();
	resnum++;
	break;
      case 3:
	// this residue actually belongs to the previous one
	resnum--;
        addEnd(lt, mtb.be(index), resnum);
        addCovEnd(lt,mtb.be(index),lt.atoms().size()-mtb.be(index).numAtoms());
	resnum++;
	break;
      }
    }
    
    // this would be the place to handle any cysteine bridges
    for(unsigned int j=0; j<cys1.size(); j++)
      for(unsigned int k=0; k<lt.resMap().size();k++)
	if(lt.resMap()[k]==cys1[j]&&lt.atoms()[k].name()=="CA") 
	  {csa1.push_back(k); break;}
    
    for(unsigned int j=0; j<cys2.size(); j++)
      for(unsigned int k=0; k<lt.resMap().size();k++)
	if(lt.resMap()[k]==cys2[j]&&lt.atoms()[k].name()=="CA") 
	  {csa2.push_back(k); break;}
    
    for(unsigned int j=0; j<csa1.size(); j++)
      setCysteines(lt, csa1[j], csa2[j]);
    
    // possibly cyclize
    if(cyclic) cyclize(lt);
    
    // get the 1-4 interactions from the bonds
    lt.get14s();

    // transform masses from integer to double
    for(unsigned int i=0; i< lt.atoms().size(); i++)
      lt.atoms()[i].setMass(gff.findMass(int(lt.atoms()[i].mass())));
      
    // parse everything into a system    
    System sys;
    lt.parse(sys);
    
    // add the solvent topology
    index=mtb.findBs(args["solv"]);
    if(index==0) throw gromos::Exception("maketop", 
		"Cannot find building block for "
		 +args["solv"]+" in "+args["build"]);
    
    SolventTopology st;

    // adapt the masses
    for(int i=0; i<mtb.bs(index-1).numAtoms(); i++){
      AtomTopology at=mtb.bs(index-1).atom(i);
      at.setMass(gff.findMass(int(at.mass())));
      st.addAtom(at);
    }
    ConstraintIterator cit(mtb.bs(index-1));
    for(;cit;++cit)
      st.addConstraint(cit());
    st.setSolvName(mtb.bs(index-1).solvName());
    
    sys.addSolvent(Solvent(st));
 
    // we have to determine still what is a H and what not
    for(int m=0; m<sys.numMolecules(); m++){
      sys.mol(m).topology().clearH();
      sys.mol(m).topology().setHmass(1.008);
    }
    
    // write the topology
    OutTopology ot(cout);
    string title;
    title+="MAKETOP topology, using:\n"+args["build"]+"\n"+args["param"];
    ot.setTitle(title);

    // set the physical constants in the gff    
    gff.setFpepsi(mtb.Fpepsi());
    gff.setHbar(mtb.Hbar());

   
    ot.write(sys,gff);
    
  }
  catch(gromos::Exception e){
    cerr << e.what() << endl;
    return 1;
  }
}
