// gcore_BbSolute.cc

#include <cassert>
#include "BbSolute.h"
#include "AtomTopology.h"
#include "Exclusion.h"
#include "Bond.h"
#include "Angle.h"
#include "Improper.h"
#include "Dihedral.h"
#include <set>
#include <vector>
#include <map>
#include <new>

using namespace std;
using gcore::BbSolute;
using gcore::MoleculeTopology;

using gcore::Bond;
using gcore::Angle;
using gcore::Dihedral;
using gcore::Improper;
using gcore::AtomTopology;
using gcore::Exclusion;


BbSolute::BbSolute(const BbSolute& mt)
{
  for(int i=0; i<mt.numAtoms(); i++)
    MoleculeTopology::addAtom(mt.atom(i));
  for(int i=0; i<mt.numPexcl(); i++)
    addPexcl(mt.pexcl(i));
  BondIterator bi(mt);
  for(;bi;++bi)
    MoleculeTopology::addBond(bi());
  AngleIterator ai(mt);
  for(;ai;++ai)
    MoleculeTopology::addAngle(ai());
  DihedralIterator di(mt);
  for(;di;++di)
    MoleculeTopology::addDihedral(di());
  ImproperIterator ii(mt);
  for(;ii;++ii)
    MoleculeTopology::addImproper(ii());
  setResName(mt.resName());
  setRep(mt.rep());
}

// Methods

BbSolute &BbSolute::operator=(const BbSolute &mt){
  if (this != &mt){
    this->BbSolute::~BbSolute();
    new(this) BbSolute(mt);
  }
  return *this;
}

void BbSolute::addPexcl(const Exclusion &a)
{
  d_pexcl.push_back(a);
  return;
}

void BbSolute::setResName(const string &s){
  MoleculeTopology::setResName(0,s);
}

int BbSolute::numPexcl()const{return d_pexcl.size();}

const Exclusion &BbSolute::pexcl(int i)const
{
  assert(i < int(d_pexcl.size()));
  return d_pexcl[i];
}

const string &BbSolute::resName()const{
  return MoleculeTopology::resName(0);
}
void BbSolute::setRep(int i)
{
  d_rep=i;
}

const int BbSolute::rep()const{return d_rep;} 






