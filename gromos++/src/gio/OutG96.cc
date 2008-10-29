// gio_OutG96.cc

#include <cassert>
#include "OutG96.h"
#include "../gromos/Exception.h"
#include "../gcore/System.h"
#include "../gcore/Molecule.h"
#include "../gcore/Solvent.h"
#include "../gcore/MoleculeTopology.h"
#include "../gcore/SolventTopology.h"
#include "../gcore/AtomTopology.h"
#include "../gmath/Vec.h"
#include "../gcore/Box.h"
#include "../gcore/Remd.h"
#include "../args/Arguments.h"
#include "../utils/Time.h"
#include <iostream>
#include <iomanip>

using gio::OutG96;
using gio::OutG96_i;
using namespace gcore;
using namespace std;

class OutG96_i{
  friend class gio::OutG96;
  ostream &d_os;
  int d_count;
  int d_switch;
  OutG96_i(ostream &os):
    d_os(os), d_count(0)
  {d_switch = 0;}
  ~OutG96_i(){}

  void select(const string &thing);
  void writeTrajM(const Molecule &mol);
  void writeTrajS(const Solvent &sol);
  void writeBox(const Box &box);
  void writeTriclinicBox(const Box &box);
  void writeGenBox(const Box &box);
  void writeRemd(const Remd &remd);
};

OutG96::OutG96(ostream &os):
  OutCoordinates(),
  d_this(new OutG96_i(os)){}
OutG96::OutG96():
  OutCoordinates()
{d_this=0;}

OutG96::~OutG96(){
  if(d_this)delete d_this;
}

void OutG96::writeTitle(const string &title){
  d_this->d_os << "TITLE\n" << title << "\nEND\n";
}

void OutG96::writeTimestep(const int step, const double time)
{
  d_this->d_os.precision(9);
  d_this->d_os.setf(std::ios::fixed, std::ios::floatfield);
  
  d_this->d_os << "TIMESTEP\n"
	       << std::setw(15)
	       << step
	       << std::setw(15)
	       << time
	       << "\nEND\n";
}

void OutG96::select(const string &thing){
  if (thing == "ALL"){
    d_this->d_switch = 1;
  }
  else if (thing =="SOLVENT"){
    d_this->d_switch = 2;
  }
  else {
    d_this->d_switch = 0;
  }
}

void OutG96::open(ostream &os){
  if(d_this){
    delete d_this;
  }
  d_this=new OutG96_i(os);
}

void OutG96::close(){
  if(d_this)delete d_this;
  d_this=0;
}

OutG96 &OutG96::operator<<(const gcore::System &sys){
  d_this->d_count=0;
  if(sys.hasRemd){
    d_this->d_os << "REMD\n";
    d_this->writeRemd(sys.remd());
    d_this->d_os << "END\n";
  }
  d_this->d_os << "POSITIONRED\n";
  if (d_this->d_switch <=1){
  for(int i=0;i<sys.numMolecules();++i){
    d_this->writeTrajM(sys.mol(i));}
  }
  if (d_this->d_switch >=1){
  for(int i=0;i<sys.numSolvents();++i){
    d_this->writeTrajS(sys.sol(i));}
  }
  d_this->d_os << "END\n";

  if (args::Arguments::outG96) {
    switch (sys.box().boxformat()) {
      case gcore::Box::box96 :
        d_this->d_os << "BOX\n";
        d_this->writeBox(sys.box());
        d_this->d_os << "END\n";
        break;
      case gcore::Box::triclinicbox :
        d_this->d_os << "TRICLINICBOX\n";
        d_this->writeTriclinicBox(sys.box());
        d_this->d_os << "END\n";
        break;
      case gcore::Box::genbox :
        d_this->d_os << "GENBOX\n";
        d_this->writeGenBox(sys.box());
        d_this->d_os << "END\n";
        break;
      default:
        throw gromos::Exception("OutG96", "Don't know how to handle boxformat");
    }
  } else {
    // in GXX there is only one single format called GENBOX
    d_this->d_os << "GENBOX\n";
    d_this->writeGenBox(sys.box());
    d_this->d_os << "END\n";      
  }
  
  return *this;
}

void OutG96_i::writeRemd(const Remd &remd)
{
  d_os.setf(ios::fixed, ios::floatfield);
  d_os.precision(1);
  d_os << setw(15) << remd.id() 
       << setw(10) << remd.run()
       << setprecision(1) << setw(10) << remd.temperature()
       << setprecision(6) << setw(10) << remd.lambda()
       << "\n"
       << setw(15) << remd.Ti()
       << setw(10) << remd.li()
       << setw(10) << remd.Tj()
       << setw(10) << remd.lj()
       << setw(10) << remd.reeval()
       << "\n";
}

void OutG96_i::writeTrajM(const Molecule &mol){
  d_os.setf(ios::fixed, ios::floatfield);
  d_os.precision(9);
  for (int i=0;i<mol.numPos();++i){
    ++d_count;
    d_os << setw(15) << mol.pos(i)[0]
	 << setw(15) << mol.pos(i)[1]
	 << setw(15) << mol.pos(i)[2]<< endl;
    if(!(d_count%10))
      d_os << "#" << setw(10)<<d_count<<endl;
  } 
}
void OutG96_i::writeTrajS(const Solvent &sol){
  d_os.setf(ios::fixed, ios::floatfield);
  d_os.precision(9);
  for (int i=0;i<sol.numPos();++i){
    ++d_count;
    d_os << setw(15) << sol.pos(i)[0]
	 << setw(15) << sol.pos(i)[1]
	 << setw(15) << sol.pos(i)[2]<< endl;
    if(!(d_count%10))
      d_os << "#" << setw(10)<<d_count<<endl;
  } 
}


void OutG96_i::writeBox(const Box &box){
  d_os.setf(ios::fixed, ios::floatfield);
  d_os.precision(9);

  d_os << setw(15) << box[0] 
       << setw(15) << box[1]
       << setw(15) << box[2] << endl;
}

void OutG96_i::writeTriclinicBox(const Box &box){
  d_os.setf(ios::fixed, ios::floatfield);
  d_os.precision(9);

  d_os << setw(8) << box.ntb() << endl;
  for(int i=0; i<3; ++i){
    d_os << setw(15) << box.K()[i] 
	 << setw(15) << box.L()[i]
	 << setw(15) << box.M()[i] << endl;
  }
}

void OutG96_i::writeGenBox(const Box &box){
  d_os.setf(ios::fixed, ios::floatfield);
  d_os.precision(9);
  const double k=box.K().abs();
  const double l=box.L().abs();
  const double m=box.M().abs();
  d_os << setw(8) << box.ntb() << endl;
  d_os << setw(15) << k
       << setw(15) << l
       << setw(15) << m << endl;
  d_os << setw(15) << acos(box.L().dot(box.M())/(l*m))*180/M_PI
       << setw(15) << acos(box.K().dot(box.M())/(k*m))*180/M_PI
       << setw(15) << acos(box.K().dot(box.L())/(k*l))*180/M_PI << endl;
  // construct a local x,y,z with x along k, y in the k,l plane and z in the direction of m

  Vec z = box.K().cross(box.L()).normalize();
  Vec x = box.K().normalize();
  Vec p,q;
  if(x[2]==0){
    p=x;
  }
  else{
    p=Vec(-z[1], z[0], 0);
    p=p.normalize();
  }
  q = -p.cross(z);
  
  double phi = acos (p.dot(x))*180/M_PI;
  double theta = asin(q[2])*180/M_PI;
  double psi = asin(p[1])*180/M_PI;

  d_os << setw(15) << phi 
       << setw(15) << theta
       << setw(15) << psi << endl;
}
