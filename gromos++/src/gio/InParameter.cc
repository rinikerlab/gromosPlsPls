// gio_InParameter.cc

#include <cassert>
#include "InParameter.h"
#include "Ginstream.h"
#include "../gcore/MassType.h"
#include "../gcore/BondType.h"
#include "../gcore/AngleType.h"
#include "../gcore/DihedralType.h"
#include "../gcore/ImproperType.h"
#include "../gcore/LJType.h"
#include "../gcore/AtomPair.h"
#include "../gcore/AtomTopology.h"
#include "../gcore/GromosForceField.h"

#include <map>
#include <vector>
#include <string>

using namespace std;
using namespace gcore;
using gio::InParameter_i;
using gio::InParameter;

// Implementation class
class InParameter_i: public gio::Ginstream
{
  friend class gio::InParameter;
  gcore::GromosForceField d_gff;
  std::map<std::string, std::vector<std::string> > d_blocks;
  /**
   * The init function reads in the whole file into the map of blocks.
   */
  void init();
  /**
   * _initBlock is a function that initializes the reading of
   * a block. It checks whether the block is read in, and returns
   * the number of lines that are to be read in from it.
   */
  int _initBlock(std::vector<std::string> &buffer,
		 std::vector<std::string>::const_iterator &it,
		 const string blockname);
  /**
   * parseForceField takes all relevant blocks and stores the information in
   * d_gff
   */
  void parseForceField();
  
  InParameter_i::InParameter_i (std::string &s): d_gff(), d_blocks()
  {
    this->open(s);
    this->init();
    this->parseForceField();
  }
};

// Constructors

gio::InParameter::InParameter(std::string name){
  d_this = new InParameter_i(name);
}

gio::InParameter::~InParameter(){
  delete d_this;
}

const std::string gio::InParameter::title()const{
  return d_this->title();
}

const gcore::GromosForceField &gio::InParameter::forceField()const{
  return d_this->d_gff;
}

int gio::InParameter_i::_initBlock(std::vector<std::string> &buffer,
				  std::vector<std::string>::const_iterator &it,
				   const string blockname)
{
  buffer.clear();
  buffer=d_blocks[blockname];
  if(buffer.size() < 3)
    throw InParameter::Exception("Parameter file "+name()+
				" is corrupted. No (or empty) "+blockname+
				" block!");
    if(buffer[buffer.size()-1].find("END")!=0)
      throw InParameter::Exception("Topology file " + name() +
				       " is corrupted. No END in "+blockname+
				       " block. Got\n"
				       + buffer[buffer.size()-1]);

  it=buffer.begin()+1;
  return buffer.size()-2;
}

void gio::InParameter_i::init(){

  if(!stream())
    throw InParameter::Exception("Could not open parameter file "+name());

  // First read the whole file into the map
  std::vector<std::string> buffer;
  
  while(!stream().eof()){
    getblock(buffer);
    if(buffer.size()){
      d_blocks[buffer[0]] = buffer;
      buffer.clear();
    }
  }
}


void gio::InParameter_i::parseForceField()
{
  // generic variables
  double d[4];
  int num,n,i[5];
  string s;
  std::vector<std::string> buffer;
  std::vector<std::string>::const_iterator it;
  
  { // FORCEFIELD block
    buffer.clear();
    buffer=d_blocks["FORCEFIELD"];
    if(buffer.size()>0){
      if(buffer.size() != 3) 
	throw InParameter::Exception("Parameter file " + name() +
				     " is corrupted. FORCEFIELD block should have only "
				     "one line");
      if(buffer[buffer.size()-1].find("END")!=0)
	throw InParameter::Exception("Parameter file " + name() +
				     " is corrupted. No END in FORCEFIELD"
				     " block. Got\n"
				     + buffer[buffer.size()-1]);

      d_gff.setForceField(buffer[1]);
    }
  }
  
  
    
  { // MASSATOMTYPECODE block
    num = _initBlock(buffer, it, "MASSATOMTYPECODE");
    for(n=0; n<num; ++it, ++n){
      _lineStream.clear();
      _lineStream.str(*it);
      _lineStream >> i[0] >> d[0] >> s;
      if(_lineStream.fail())
	throw InParameter::Exception("Bad line in MASSATOMTYPECODE block:\n"
				     +*it);
      d_gff.addMassType(MassType(--i[0], d[0]));
    }
  } // MASSATOMTYPECODE block
  { // BONDTYPECODE block
    num = _initBlock(buffer, it, "BONDTYPECODE");
    for(n=0; n<num; ++it, ++n){
      _lineStream.clear();
      _lineStream.str(*it);
      _lineStream >> i[0] >> d[0] >> d[1];
      if(_lineStream.fail())
	throw InParameter::Exception("Bad line in BONDTYPECODE block:\n"
				     +*it);
      if(i[0]!=n+1)
	throw InParameter::Exception(
	     "BondTypes in BONDTYPECODE block are not sequential");
  
      d_gff.addBondType(BondType(d[0], d[1]));
    }
  } // BONDTYPECODE block
  { // BONDANGLETYPECOD block
    num = _initBlock(buffer, it, "BONDANGLETYPECOD");
    for(n=0; n<num; ++it, ++n){
      _lineStream.clear();
      _lineStream.str(*it);
      _lineStream >> i[0] >> d[0] >> d[1];
      if(_lineStream.fail())
	throw InParameter::Exception("Bad line in BONDANGLETYPECOD block:\n"
				     +*it);
      if(i[0]!=n+1)
	throw InParameter::Exception(
	     "AngleTypes in BONDANGLETYPECOD block are not sequential");
  
      d_gff.addAngleType(AngleType(d[0],d[1]));
    }
  } //BONDANGLETYPECOD
  { // IMPDIHEDRALTYPEC block
    num = _initBlock(buffer, it, "IMPDIHEDRALTYPEC");
    for(n=0; n<num; ++it, ++n){
      _lineStream.clear();
      _lineStream.str(*it);
      _lineStream >> i[0] >> d[0] >> d[1];
      if(_lineStream.fail())
	throw InParameter::Exception("Bad line in IMPDIHEDRALTYPEC block:\n"
				     +*it);
      if(i[0]!=n+1)
	throw InParameter::Exception(
	     "ImproperTypes in IMPDIHEDRALTYPEC block are not sequential");
      d_gff.addImproperType(ImproperType(d[0],d[1]));
    }
  } // IMPDIHEDRALTYPEC 
  { // DIHEDRALTYPECODE block
    num = _initBlock(buffer, it, "DIHEDRALTYPECODE");
    for(n=0; n<num; ++it, ++n){
      _lineStream.clear();
      _lineStream.str(*it);
      _lineStream >> i[0] >> d[0] >> d[1] >> i[1];
      if(_lineStream.fail())
	throw InParameter::Exception("Bad line in DIHEDRALTYPECODE block:\n"
				     +*it);
      if(i[0]!=n+1)
	throw InParameter::Exception(
	     "DihedralTypes in DIHEDRALTYPECODE block are not sequential");
      d_gff.addDihedralType(DihedralType(d[0], d[1], i[1]));
    }
  } // DIHEDRALTYPECODE
  { // SINGLEATOMLJPAIR
    num = _initBlock(buffer, it, "SINGLEATOMLJPAIR");
    _lineStream.clear();
    _lineStream.str(*it);
    _lineStream >> num;
    std::vector<double> sc6(num), sc12[3], scs6(num), scs12(num);
    sc12[0].resize(num);
    sc12[1].resize(num);
    sc12[2].resize(num);
    
    std::vector<std::vector<int> >    pl(num);
    for(int j=0; j<num; ++j){
      pl[j].resize(num);
    }
    std::string ljblock;
    gio::concatenate(it+1, buffer.end()-1, ljblock);
    _lineStream.clear();
    _lineStream.str(ljblock);
    for(n=0; n<num; n++){
      _lineStream >> i[0]>> s>> sc6[n]>> sc12[0][n]>> sc12[1][n]>> sc12[2][n];
      _lineStream >> scs6[n] >> scs12[n];
      if(_lineStream.fail()){
	ostringstream os;
	os << "Bad line in SINGLEATOMLJPAIR block, IAC: " << n+1 << "\n"
	   << "Trying to read parameters";
	throw InParameter::Exception(os.str());
      }
      if(i[0]!=n+1)
	throw InParameter::Exception(
	     "AtomTypes in SINGLEATOMLJPAIR block are not sequential");

      for(int k=0; k<num; k++)
	_lineStream >> pl[n][k];
      if(_lineStream.fail()){
	ostringstream os;
	os << "Bad line in SINGLEATOMLJPAIR block, IAC: " << n+1 << "\n"
	   << "Trying to read " << num << " elements of the interaction "
	   << "matrix";
	throw InParameter::Exception(os.str());
      }
      d_gff.addAtomTypeName(s);
      for(int k=0; k<=n; k++){
	d[1] = sc6[n]              * sc6[k];
	d[0] = sc12[pl[n][k]-1][n] * sc12[pl[k][n]-1][k];
	d[3] = scs6[n]             * scs6[k];
	d[2] = scs12[n]            * scs12[k];
	d_gff.setLJType(AtomPair(n,k), LJType(d[0], d[1], d[2], d[3]));
      }
    }
  } // SINGLEATOMLJPAIR
  { // MIXEDATOMLJPAIR block
    // this one does not have to be there
    if(d_blocks.count("MIXEDATOMLJPAIR")){
      
      num = _initBlock(buffer, it, "MIXEDATOMLJPAIR");
      for(n =0; n<num; ++it, ++n){
	_lineStream.clear();
	_lineStream.str(*it);
	_lineStream >> i[0] >> i[1] >> d[1] >> d[0] >> d[3] >> d[2];
	if(_lineStream.fail())
	  throw InParameter::Exception("Bad line in MIXEDATOMLJPAIR block:\n"
				       +*it);
	d_gff.setLJType(AtomPair(--i[0],--i[1]),LJType(d[0],d[1],d[2],d[3]));
      }
    } // MIXEDATOMLJPAIR
  }
  
}




