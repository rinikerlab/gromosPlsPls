// gcore_MoleculeTopology.cc
#include <cassert>
#include "MoleculeTopology.h"
#include "AtomTopology.h"
#include "Bond.h"
#include "Angle.h"
#include "Improper.h"
#include "Dihedral.h"
#include <set>
#include <vector>
#include <map>
#include <new>

using namespace std;
using gcore::MoleculeTopology_i;
using gcore::MoleculeTopology;
using gcore::BondIterator;
using gcore::AngleIterator;
using gcore::DihedralIterator;
using gcore::ImproperIterator;
using gcore::BondIterator_i;
using gcore::AngleIterator_i;
using gcore::DihedralIterator_i;
using gcore::ImproperIterator_i;
using gcore::Bond;
using gcore::Angle;
using gcore::Dihedral;
using gcore::Improper;
using gcore::AtomTopology;

class MoleculeTopology_i{

  friend class MoleculeTopology;
  friend class BondIterator;
  friend class AngleIterator;
  friend class DihedralIterator;
  friend class ImproperIterator;

  vector<AtomTopology> d_atoms;
  set<Bond> d_bonds;
  set<Angle> d_angles;
  set<Dihedral> d_dihedrals;
  set<Improper> d_impropers;
  vector<string> d_resNames;
  map<int,int> d_resNums;
  MoleculeTopology_i():
    d_atoms(),
    d_bonds(),
    d_angles(),
    d_dihedrals(),
    d_impropers(),
    d_resNames(),
    d_resNums()
  {}
  ~MoleculeTopology_i(){}
};


MoleculeTopology::MoleculeTopology() : 
  d_this(new MoleculeTopology_i())
{}

MoleculeTopology::MoleculeTopology(const MoleculeTopology& mt):
  d_this(new MoleculeTopology_i())
{
  d_this->d_atoms=(mt.d_this->d_atoms);
  d_this->d_bonds=(mt.d_this->d_bonds);
  d_this->d_angles=(mt.d_this->d_angles);
  d_this->d_dihedrals=(mt.d_this->d_dihedrals);
  d_this->d_impropers=(mt.d_this->d_impropers);
  d_this->d_resNames=(mt.d_this->d_resNames);
  d_this->d_resNums=(mt.d_this->d_resNums);
}

MoleculeTopology::~MoleculeTopology(){delete d_this;}

// Methods

MoleculeTopology &MoleculeTopology::operator=(const MoleculeTopology &mt){
  if (this != &mt){
    this->MoleculeTopology::~MoleculeTopology();
    new(this) MoleculeTopology(mt);
  }
  return *this;
}

void MoleculeTopology::addAtom(const AtomTopology &a){
  d_this->d_atoms.push_back(a);
  return;
}

void MoleculeTopology::addBond(const Bond &b){
  // add checks if bond there?
  d_this->d_bonds.insert(b);
}

void MoleculeTopology::addAngle(const Angle &a){
  // add checks if angle there?
  d_this->d_angles.insert(a);
}

void MoleculeTopology::addDihedral(const Dihedral &a){
  // add checks if dihedral there?
  d_this->d_dihedrals.insert(a);
}

void MoleculeTopology::addImproper(const Improper &a){
  // add checks if improper there?
  d_this->d_impropers.insert(a);
}

void MoleculeTopology::setResName(int res, const string &s){
  int num=d_this->d_resNames.size();
  if(res < num){
    d_this->d_resNames[res]=s;
  }
  else{
    for(int i=num; i<res; ++i)
      d_this->d_resNames.push_back(string());
    d_this->d_resNames.push_back(s);
  }
}

void MoleculeTopology::setResNum(int atom, int res){
  d_this->d_resNums[atom]=res;
}

void MoleculeTopology::clearH()
{
  for(unsigned int i=0;i < d_this->d_atoms.size(); i++)
    d_this->d_atoms[i].setH(false);
}

void MoleculeTopology::setHmass(double mass)
{
  for(unsigned int i=0; i< d_this->d_atoms.size(); i++)
    if(d_this->d_atoms[i].mass() == mass)
      d_this->d_atoms[i].setH(true);
}

void MoleculeTopology::setHiac(int iac)
{
   for(unsigned int i=0; i< d_this->d_atoms.size(); i++)
    if(d_this->d_atoms[i].iac() == iac)
      d_this->d_atoms[i].setH(true);
}

int MoleculeTopology::numAtoms()const{return d_this->d_atoms.size();}

AtomTopology &MoleculeTopology::atom(int i)
{
  assert(i < int(d_this->d_atoms.size()));
  return d_this->d_atoms[i];
}

const AtomTopology &MoleculeTopology::atom(int i)const{
  assert(i < int(d_this->d_atoms.size()));
  return d_this->d_atoms[i];
}

int MoleculeTopology::numRes()const{
  return d_this->d_resNames.size();}

int MoleculeTopology::resNum(int i)const{
  assert(i < int(d_this->d_resNums.size()));
  return d_this->d_resNums[i];
}

const string &MoleculeTopology::resName(int i)const{
  assert(i < int(d_this->d_resNames.size()));
  return d_this->d_resNames[i];
}

class BondIterator_i{
  friend class BondIterator;
  set<Bond>::iterator d_it;
  const MoleculeTopology *d_mt;
  // not implemented
  BondIterator_i(const BondIterator_i&);
  BondIterator_i &operator=(const BondIterator_i &);
public:
  BondIterator_i():
    d_it(){d_mt=0;}
};

BondIterator::BondIterator(const MoleculeTopology &mt):
  d_this(new BondIterator_i())
{
  d_this->d_it=mt.d_this->d_bonds.begin();
  d_this->d_mt=&mt;
}

BondIterator::~BondIterator(){delete d_this;}

void BondIterator::operator++(){
  ++(d_this->d_it);
}

const Bond &BondIterator::operator()()const{
  return *(d_this->d_it);
}

BondIterator::operator bool()const{
  return d_this->d_it != d_this->d_mt->d_this->d_bonds.end();
}

class AngleIterator_i{
  friend class AngleIterator;
  set<Angle>::iterator d_it;
  const MoleculeTopology *d_mt;
  // not implemented
  AngleIterator_i(const AngleIterator_i&);
  AngleIterator_i &operator=(const AngleIterator_i &);
public:
  AngleIterator_i():
    d_it(){d_mt=0;}
};

AngleIterator::AngleIterator(const MoleculeTopology &mt):
  d_this(new AngleIterator_i())
{
  d_this->d_it=mt.d_this->d_angles.begin();
  d_this->d_mt=&mt;
}

AngleIterator::~AngleIterator(){delete d_this;}

void AngleIterator::operator++(){
  ++(d_this->d_it);
}

const Angle &AngleIterator::operator()()const{
  return *(d_this->d_it);
}

AngleIterator::operator bool()const{
  return d_this->d_it != d_this->d_mt->d_this->d_angles.end();
}

class ImproperIterator_i{
  friend class ImproperIterator;
  set<Improper>::iterator d_it;
  const MoleculeTopology *d_mt;
  // not implemented
  ImproperIterator_i(const ImproperIterator_i&);
  ImproperIterator_i &operator=(const ImproperIterator_i &);
public:
  ImproperIterator_i():
    d_it(){d_mt=0;}
};

ImproperIterator::ImproperIterator(const MoleculeTopology &mt):
  d_this(new ImproperIterator_i())
{
  d_this->d_it=mt.d_this->d_impropers.begin();
  d_this->d_mt=&mt;
}

ImproperIterator::~ImproperIterator(){delete d_this;}

void ImproperIterator::operator++(){
  ++(d_this->d_it);
}

const Improper &ImproperIterator::operator()()const{
  return *(d_this->d_it);
}

ImproperIterator::operator bool()const{
  return d_this->d_it != d_this->d_mt->d_this->d_impropers.end();
}

class DihedralIterator_i{
  friend class DihedralIterator;
  set<Dihedral>::iterator d_it;
  const MoleculeTopology *d_mt;
  // not implemented
  DihedralIterator_i(const DihedralIterator_i&);
  DihedralIterator_i &operator=(const DihedralIterator_i &);
public:
  DihedralIterator_i():
    d_it(){d_mt=0;}
};

DihedralIterator::DihedralIterator(const MoleculeTopology &mt):
  d_this(new DihedralIterator_i())
{
  d_this->d_it=mt.d_this->d_dihedrals.begin();
  d_this->d_mt=&mt;
}

DihedralIterator::~DihedralIterator(){delete d_this;}

void DihedralIterator::operator++(){
  ++(d_this->d_it);
}

const Dihedral &DihedralIterator::operator()()const{
  return *(d_this->d_it);
}

DihedralIterator::operator bool()const{
  return d_this->d_it != d_this->d_mt->d_this->d_dihedrals.end();
}



