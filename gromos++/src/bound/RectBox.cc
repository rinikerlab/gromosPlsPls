// bound_RectBox.cc

#include "RectBox.h"
#include "../gmath/Vec.h"
#include "../gcore/System.h"
#include "../gcore/Molecule.h"
#include "../gcore/Solvent.h"
#include "../gcore/SolventTopology.h"
#include "../gcore/Box.h"
#include "../fit/PositionUtils.h"

#include <cmath>
#include <iostream>

using namespace std;
using bound::RectBox;
using gmath::Vec;
using namespace gcore;

static Vec nim(const Vec &r1,const  Vec &r2, const Box &box){

  Vec diff=r2-r1;
  Vec a;
  
  a[0] = diff[0] - box[0] * rint(diff[0]/box[0]);
  a[1] = diff[1] - box[1] * rint(diff[1]/box[1]);
  a[2] = diff[2] - box[2] * rint(diff[2]/box[2]);


  Vec rec = r1 + a;
  
  return rec;
  
}


Vec RectBox::nearestImage(const Vec &v1, const Vec &v2, const Box &box)const{
  return nim(v1, v2, box);
}

void RectBox::nogather(){

}

void RectBox::gather(){

  if (!sys().hasBox) throw gromos::Exception("Gather problem",  
                              "System does not contain Box block! Abort!");

  if (sys().box()[0] == 0 || sys().box()[1] == 0 || sys().box()[2] == 0) 
    throw gromos::Exception("Gather problem",  
			    "Box block contains element(s) of value 0.0! Abort!");  

  for(int i=0; i<sys().numMolecules();++i){
    Molecule &mol=sys().mol(i);
    mol.pos(0)=nim(reference(0),mol.pos(0),sys().box());
    for(int j=1;j<mol.numPos();++j)
      mol.pos(j)=nim(mol.pos(j-1),mol.pos(j),sys().box());
  }
  // do the solvent 
  Solvent &sol=sys().sol(0);
  for(int i=0;i<sol.numPos();i+= sol.topology().numAtoms()){
    sol.pos(i)=nim(reference(0),sol.pos(i),sys().box());  
    for (int j=i+1;j < (i+sol.topology().numAtoms());++j){
      sol.pos(j)=nim(sol.pos(j-1),sol.pos(j),sys().box());
    }
  }
}

void RectBox::gathergr(){

  if (!sys().hasBox) 
    throw gromos::Exception("Gather problem",  
			    "System does not contain Box block! Abort!");

   if (sys().box()[0] == 0 || sys().box()[1] == 0 || sys().box()[2] == 0)
     throw gromos::Exception("Gather problem",  
			     "Box block contains element(s) of value 0.0! Abort!");
   
   for(int i=0; i<sys().numMolecules();++i){
     Molecule &mol=sys().mol(i);
     mol.pos(0)=nim(reference(i),mol.pos(0),sys().box());
     for(int j=1;j<mol.numAtoms();++j)
       mol.pos(j)=nim(mol.pos(j-1),mol.pos(j),sys().box());
   }
}

void RectBox::gathermgr(){

  if (!sys().hasBox) 
    throw gromos::Exception("Gather problem",  
			    "System does not contain Box block! Abort!");

   if (sys().box()[0] == 0 || sys().box()[1] == 0 || sys().box()[2] == 0)
     throw gromos::Exception("Gather problem",  
			     "Box block contains element(s) of value 0.0! Abort!");
   
   const Vec centre(0.5 * sys().box()[0], 0.5 * sys().box()[1], 0.5 * sys().box()[2]);

   for(int i=0; i<sys().numMolecules();++i){
     Molecule &mol=sys().mol(i);
     mol.pos(0)=nim(reference(i),mol.pos(0),sys().box());
     for(int j=1;j<mol.numAtoms();++j)
       mol.pos(j)=nim(mol.pos(j-1),mol.pos(j),sys().box());

     // now calculate cog
     Vec cog(0.0, 0.0, 0.0);
     for(int a=0; a<mol.numAtoms(); ++a)
       cog += mol.pos(a);
     cog /= mol.numAtoms();
     Vec cog_box = nim(centre, cog, sys().box());
     Vec trans = cog_box - cog;
     fit::PositionUtils::translate(mol, trans);
   }
}


void RectBox::coggather(){

  if (!sys().hasBox)
    throw gromos::Exception("Gather problem",  
			    "System does not contain Box block! Abort!");
  
  if (sys().box()[0] == 0 || sys().box()[1] == 0 || sys().box()[2] == 0)
    throw gromos::Exception("Gather problem",  
			    "Box block contains element(s) of value 0.0! Abort!");
  
  Molecule &mol=sys().mol(0);
  Solvent &sol=sys().sol(0);
  
  Vec ref(0.0,0.0,0.0);
  Vec cog;
  int atoms=0;
  
  // do mol(0) with respect to ref (0,0,0)
  mol.pos(0)=nim(ref,mol.pos(0),sys().box());
  for(int j=1;j<mol.numAtoms();++j){
    mol.pos(j)=nim(mol.pos(j-1),mol.pos(j),sys().box());}
  
  // calculate COG of mol(0)
  for (int i=0;i < mol.numAtoms(); i++) {
    cog = cog + mol.pos(i);
    ++atoms;
  }
  cog = (1.0/double(atoms))*cog;
  
  // do the rest of the molecules
  for(int i=1;i<sys().numMolecules();++i){
    Molecule &mol=sys().mol(i);
    mol.pos(0)=nim(cog,mol.pos(0),sys().box());      
    for(int j=1;j<mol.numPos();++j){
      mol.pos(j)=nim(mol.pos(j-1),mol.pos(j),sys().box());
    }
  }
  
  // do the solvent 
  for(int i=0;i<sol.numPos();i+= sol.topology().numAtoms()){
    sol.pos(i)=nim(cog,sol.pos(i),sys().box());  
    for (int j=i+1;j < (i+sol.topology().numAtoms());++j){
      sol.pos(j)=nim(sol.pos(j-1),sol.pos(j),sys().box());
    }
  }
  
} 

void RectBox::gengather(){

  if (!sys().hasBox)
    throw gromos::Exception("Gather problem",
               "System does not contain Box block! Abort!");

  if (sys().box()[0] == 0 || sys().box()[1] == 0 || sys().box()[2] == 0)
    throw gromos::Exception("Gather problem",
                "Box block contains element(s) of value 0.0! Abort!");

  // Reconstruct the connectivity of the submolecules
  for(int i=0; i<sys().numMolecules();++i){
    Molecule &mol=sys().mol(i);
    mol.pos(0)=nim(reference(i),mol.pos(0),sys().box());
    for(int j=1;j<mol.numAtoms();++j)
      mol.pos(j)=nim(mol.pos(j-1),mol.pos(j),sys().box());
  }

  // Determine the positions of the centres of geometry of the gathered molecules 
  // and store them in vcog
  vector<Vec> vcog;
  for(int i=0; i<sys().numMolecules();++i){
    Vec cog(0.0,0.0,0.0);
    int numat=0;
    for (int j=0; j<sys().mol(i).numAtoms(); ++j) {
      cog = cog + sys().mol(i).pos(j);
      ++numat;
    }
    cog = (1.0/double(numat))*cog;
    vcog.push_back(cog);
  }

  // Gather nearest image of cog of molecule 1 w.r.t. origin
  // vcog[0]=nim((0.0,0.0,0.0),vcog[0],sys().box());

  // In the following, keep sequence of molecule numbers as stored in vcog
  vector<int> vcogi;
  for(int i=0; i<sys().numMolecules()-1;++i){
    vcogi.push_back(i);
  }

  // Now gather cog's w.r.t. each other
  // Buffer to store the overall cog of already gathered cog's
  Vec ocog=vcog[0];
  for(int i=0; i<sys().numMolecules()-1;++i){
    // Determine closest nim to i among remaining molecules
    // If necessary, swap closest j with i+1 within vcog,
    // such that the following loop always loops over
    // molecules which have no neighbour assigned yet
    Vec nimcogi=nim(vcog[i],vcog[i+1],sys().box());
    int inimcogi=i+1;
    for(int j=i+2; j<sys().numMolecules();++j){
      if(nim(vcog[i],vcog[j],sys().box()).abs()<nimcogi.abs()){
        nimcogi=nim(vcog[i],vcog[j],sys().box());
        inimcogi=j;
      }
    }
    // Now swap inimcogi with i+1
    Vec bufcog=vcog[inimcogi];
    vcog[inimcogi]=vcog[i+1];
    vcogi[i+1]=inimcogi;
    vcogi[inimcogi]=i+1;
    // Set vcog[i+1] either to its nim to vcog[i], or to
    // nim to overall cog of molecules[1 ... i], depending
    // on what corresponds with the closest distance
    Vec nic1=nim(vcog[i],vcog[i+1],sys().box());
    Vec nic2=nim(ocog/double(i+1),vcog[i+1],sys().box());
    if((nic1-vcog[i]).abs()<(nic1-vcog[i]).abs()){
      vcog[i+1]=nic1;
    }
    else{
      vcog[i+1]=nic2;
    }
    ocog+=vcog[i+1]; 
  }
  // Now gather the atoms of the solute molecules with
  // the newly determined cog of the respective molecule
  // as a reference
  for(int i=0;i<sys().numMolecules();++i){
    Molecule &mol=sys().mol(i);
    for(int j=0;j<mol.numAtoms();++j){
      mol.pos(j)=nim(vcog[vcogi[i]],mol.pos(j),sys().box());
    }
  }

  // Gather the solvent molecules with ocog as a reference
  ocog=ocog/double(sys().numMolecules());
  Solvent &sol=sys().sol(0);
  for(int i=0;i<sol.numPos();i+=sol.topology().numAtoms()){
    sol.pos(i)=nim(ocog,sol.pos(i),sys().box());
    for(int j=i+1;j<(i+sol.topology().numAtoms());++j){
      sol.pos(j)=nim(sol.pos(j-1),sol.pos(j),sys().box());
    }
  }
}
