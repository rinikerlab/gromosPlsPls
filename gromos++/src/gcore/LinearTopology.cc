// gcore_LinearTopology.cc

#include <cassert>
#include <string>
#include <set>
#include <vector>
#include <map>
#include <new>
#include <iostream>
#include "Molecule.h"
#include "MoleculeTopology.h"
#include "AtomTopology.h"
#include "Exclusion.h"
#include "Bond.h"
#include "Angle.h"
#include "Improper.h"
#include "Dihedral.h"
#include "System.h"
#include "LinearTopology.h"

using namespace std;
using namespace gcore;


LinearTopology::LinearTopology(){};

LinearTopology::LinearTopology(gcore::System &sys)
{
  int lastAtom=0;
  int lastResidue=0;

  for(int m=0; m< sys.numMolecules(); m++){
    for(int a=0; a<sys.mol(m).numAtoms(); a++){
      AtomTopology at=sys.mol(m).topology().atom(a);
      Exclusion ex, ex14;
      for(int e=0; e< sys.mol(m).topology().atom(a).exclusion().size(); e++){
	ex.insert(sys.mol(m).topology().atom(a).
		  exclusion().atom(e)+lastAtom);
      }
      for(int e=0; e< sys.mol(m).topology().atom(a).exclusion14().size(); 
	  e++){
	ex14.insert(sys.mol(m).topology().atom(a).
		  exclusion14().atom(e)+lastAtom);
      }
      at.setExclusion(ex);
      at.setExclusion14(ex14);
      
      d_atom.push_back(at);
      d_resmap[a+lastAtom]=sys.mol(m).topology().resNum(a)+lastResidue;
    }
    for(int i=0; i<sys.mol(m).topology().numRes(); i++)
      d_resname.push_back(sys.mol(m).topology().resName(i));

    BondIterator bi(sys.mol(m).topology());
    for(; bi; ++bi){
      Bond b=bi();
      b[0]+=lastAtom; b[1]+=lastAtom;
      d_bond.insert(b);
    }

    AngleIterator ai(sys.mol(m).topology());
    for(; ai; ++ai){
      Angle a=ai();
      a[0]+=lastAtom; a[1]+=lastAtom; a[2]+=lastAtom;
      d_angle.insert(a);
    }

    DihedralIterator di(sys.mol(m).topology());
    for(; di; ++di){
      Dihedral d=di();
      d[0]+=lastAtom; d[1]+=lastAtom; d[2]+=lastAtom; d[3]+=lastAtom;
      d_dihedral.insert(d);
    }

    ImproperIterator ii(sys.mol(m).topology());
    for(; ii; ++ii){
      Improper i=ii();
      i[0]+=lastAtom; i[1]+=lastAtom; i[2]+=lastAtom; i[3]+=lastAtom;
      d_improper.insert(i);
    }
    
    lastResidue += sys.mol(m).topology().numRes();
    lastAtom    += sys.mol(m).numAtoms();
  }
}

LinearTopology::~LinearTopology(){};
gcore::System LinearTopology::parse()
{
  gcore::System *sys=new System();
  parse(*sys);
  return *sys;
}

void LinearTopology::parse(gcore::System &sys)
{
  // largely copied from InTopology
  // res_corr is a correction to the residue number for
  // weird cases if a molecule is split in to two, in the middle of
  // a residue
  unsigned int atomCounter=0;
  set<Bond>::const_iterator bi=d_bond.begin();
  set<Angle>::const_iterator ai=d_angle.begin();
  set<Improper>::const_iterator ii=d_improper.begin();
  set<Dihedral>::const_iterator di=d_dihedral.begin();
  
  int prevMol=0, lastAtom=0, prevMolRes=0, resCorr=0;
  MoleculeTopology *mt;
  
  while(atomCounter < d_atom.size()){
    mt=new MoleculeTopology();

    // Detect the last atom of the first molecule & add bonds:

    for( ;bi != d_bond.end() && (*bi)[0] <= lastAtom; ++bi){
      Bond bond = *bi;
      if(bond[1]>lastAtom) lastAtom=bond[1];
      bond[0] -= prevMol; bond[1] -= prevMol;
      mt->addBond(bond);
    }
    lastAtom++;

    // add Atoms
    for(; int(atomCounter) < lastAtom; atomCounter++){
      mt->addAtom(d_atom[atomCounter]);
      
      // adapt exclusions:
      Exclusion *e;
      e=new Exclusion();
      for (int l=0;l<d_atom[atomCounter].exclusion().size();++l)
        e->insert(d_atom[atomCounter].exclusion().atom(l) - prevMol);
      mt->atom(mt->numAtoms()-1).setExclusion(*e);
      delete e;
      e=new Exclusion();
      for (int l=0;l<d_atom[atomCounter].exclusion14().size();++l)
        e->insert(d_atom[atomCounter].exclusion14().atom(l)-prevMol);
      mt->atom(mt->numAtoms()-1).setExclusion14(*e);
      delete e;

      int resn=d_resmap[atomCounter]-prevMolRes;
      if(resn+resCorr<0) resCorr -= resn;
      
      mt->setResNum(atomCounter-prevMol,resn+resCorr);
      mt->setResName(resn+resCorr,d_resname[resn+prevMolRes]);
    }
    prevMolRes+=mt->numRes();
    
    // add Angles
    for( ; ai != d_angle.end() && (*ai)[0] < lastAtom; ++ai){
      Angle angle = *ai;
      angle[0] -= prevMol; angle[1] -= prevMol; angle[2] -= prevMol;
      mt->addAngle(angle);
    }    
    
    // add Dihedrals
    for( ; di != d_dihedral.end() && (*di)[0] < lastAtom; ++di)
    {
      Dihedral dihedral = *di;
      dihedral[0] -= prevMol; dihedral[1] -= prevMol;
      dihedral[2] -= prevMol; dihedral[3] -= prevMol;
      mt->addDihedral(dihedral);
    }
    
    // add Impropers
    for( ; ii != d_improper.end() && (*ii)[0] < lastAtom; ++ii){
      Improper improper = *ii;
      improper[0] -= prevMol; improper[1] -= prevMol;
      improper[2] -= prevMol; improper[3] -= prevMol;
      mt->addImproper(improper); 
    }
    
    
    // add the molecule to the system.
    sys.addMolecule(Molecule(*mt));
    delete mt;
    prevMol=lastAtom;
  }
}


void LinearTopology::get14s()
{
  int na=d_atom.size();

  for(int i=0; i<na; i++){
    set<int> first, second, third;
    set<Bond>::const_iterator bi1 = d_bond.begin(), bi2, bi3;
    for(bi1=d_bond.begin(); bi1 != d_bond.end(); ++bi1){
      if(i == (*bi1)[0]) first.insert((*bi1)[1]);
      if(i == (*bi1)[1]) first.insert((*bi1)[0]);
    }
    for(set<int>::const_iterator iter=first.begin(), to=first.end();
        iter!=to; ++iter){
      for(bi2=d_bond.begin(); bi2 != d_bond.end(); ++bi2){
        if(*iter == (*bi2)[0]) second.insert((*bi2)[1]);
        if(*iter == (*bi2)[1]) second.insert((*bi2)[0]);
      }
    }
    for(set<int>::const_iterator iter=second.begin(), to=second.end();
        iter!=to; ++iter){ 
      for(bi3=d_bond.begin(); bi3 != d_bond.end(); ++bi3){
        if(*iter == (*bi3)[0]) third.insert((*bi3)[1]);
        if(*iter == (*bi3)[1]) third.insert((*bi3)[0]);
      }
    }
    Exclusion e;
    for(set<int>::const_iterator iter=third.begin(), to=third.end();
        iter!=to; ++iter){
      if(i<*iter&&
         !first.count(*iter)&&
         !second.count(*iter)){
        int excl=0;
        for(int k=0; k < d_atom[i].exclusion().size();k++)
          if(*iter == d_atom[i].exclusion().atom(k)) excl=1;
        if(!excl) e.insert(*iter);
      }
    }
    d_atom[i].setExclusion14(e);
  }
}

void LinearTopology::removeAtoms()
{
  set<int> rem;
  vector<int> ren;
  int corr=0;
  for(unsigned int i=0; i< d_atom.size();i++){

    if ( d_atom[i].iac() < 0 ) {
      rem.insert(i);
      corr++;
      ren.push_back(-1);
    }
    else{
      ren.push_back(i-corr);
    }
  }
  if ( rem.size() == 0 ) return;
  // add four more to ren, in order to have a buffer
  // and why did we need this? I think for cyclization in maketop
  for(int i=0; i<6; i++)
    ren.push_back(d_atom.size()+i-corr);
  
  // process the properties one at a time 
  _reduceAtoms(rem, ren);
  _reduceResidues(rem, ren);
  _reduceBonds(rem, ren);
  _reduceAngles(rem, ren);
  _reduceImpropers(rem, ren);
  _reduceDihedrals(rem, ren);
}


void LinearTopology::_reduceAtoms(std::set<int> &rem, std::vector<int> &ren)
{
  int count=0;
  for(vector<AtomTopology>::iterator iter=d_atom.begin();
      iter!=d_atom.end();){
    if (rem.count(count)) d_atom.erase(iter);
    else{
      Exclusion e;
      for(int j=0; j < iter->exclusion().size(); j++){
        if(!rem.count(iter->exclusion().atom(j)))
          e.insert(ren[iter->exclusion().atom(j)]);
      }
      iter->setExclusion(e);
      ++iter;
    }
    count++;
  }
}

void LinearTopology::_reduceResidues(std::set<int> &rem, std::vector<int> &ren)
{
  // this one is a bit ugly
  map<int, int> tempMap= d_resmap;
  vector<string> tempNames;
  int lastRes=-1;
  int resNum=-1;

  map<int, int>::iterator iter=d_resmap.begin();
  map<int, int>::iterator to  =d_resmap.end();
  for(;iter!=to; ++iter)
    if(!rem.count(iter->first)){
      if(iter->second != lastRes){
        lastRes=iter->second;
        resNum++;
        tempNames.push_back(d_resname[iter->second]);
      }
      tempMap[ren[iter->first]]=resNum;
    }

  d_resmap  = tempMap;
  d_resname = tempNames;
}

void LinearTopology::_reduceBonds(std::set<int> &rem, std::vector<int> &ren)
{
  //these are a set. Changing them while looping over them will change the
  // order during the loop. Rather create a new set and copy over...
  set<Bond> newBonds;
  set<Bond>::const_iterator iter = d_bond.begin(), to=d_bond.end();
  for(; iter != to; ++iter){
    if(rem.count((*iter)[0]) == 0 && rem.count((*iter)[1]) == 0){
      Bond b(ren[(*iter)[0]], ren[(*iter)[1]]);
      b.setType(iter->type());
      newBonds.insert(b);
    }
  }
  d_bond = newBonds;
}

void LinearTopology::_reduceAngles(std::set<int> &rem, std::vector<int> &ren)
{
  //these are a set. Changing them while looping over them will change the
  // order during the loop. Rather create a new set and copy over...
  set<Angle> newAngles;
  set<Angle>::const_iterator iter = d_angle.begin(), to=d_angle.end();
  for(; iter != to; ++iter){
    if(rem.count((*iter)[0]) == 0 && rem.count((*iter)[1]) == 0 &&
       rem.count((*iter)[2]) == 0){
      Angle a(ren[(*iter)[0]], ren[(*iter)[1]], ren[(*iter)[2]]);
      a.setType(iter->type());
      newAngles.insert(a);
    }
  }
  d_angle = newAngles;
}

void LinearTopology::_reduceImpropers(std::set<int> &rem, std::vector<int> &ren)
{
  //these are a set. Changing them while looping over them will change the
  // order during the loop. Rather create a new set and copy over...
  set<Improper> newImpropers;
  set<Improper>::const_iterator iter = d_improper.begin(), to=d_improper.end();
  for(; iter != to; ++iter){
    if(rem.count((*iter)[0]) == 0 && rem.count((*iter)[1]) == 0 &&
       rem.count((*iter)[2]) == 0 && rem.count((*iter)[3]) == 0){
      Improper i(ren[(*iter)[0]], ren[(*iter)[1]], 
		 ren[(*iter)[2]], ren[(*iter)[3]]);
      i.setType(iter->type());
      newImpropers.insert(i);
    }
  }
  d_improper = newImpropers;
}


void LinearTopology::_reduceDihedrals(std::set<int> &rem, std::vector<int> &ren)
{
   //these are a set. Changing them while looping over them will change the
  // order during the loop. Rather create a new set and copy over...
  set<Dihedral> newDihedrals;
  set<Dihedral>::const_iterator iter = d_dihedral.begin(), to=d_dihedral.end();
  for(; iter != to; ++iter){
    if(rem.count((*iter)[0]) == 0 && rem.count((*iter)[1]) == 0 &&
       rem.count((*iter)[2]) == 0 && rem.count((*iter)[3]) == 0){
      Dihedral i(ren[(*iter)[0]], ren[(*iter)[1]], 
		 ren[(*iter)[2]], ren[(*iter)[3]]);
      i.setType(iter->type());
      newDihedrals.insert(i);
    }
  }
  d_dihedral = newDihedrals;
}

