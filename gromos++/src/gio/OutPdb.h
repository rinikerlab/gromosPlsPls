// gio_OutPdb.h

#ifndef INCLUDED_GIO_OUTPdb
#define INCLUDED_GIO_OUTPdb

#ifndef INCLUDED_STRING
#include<string>
#define INCLUDED_STRING
#endif

#ifndef INCLUDED_GIO_OUTCOORDINATES
#include "OutCoordinates.h"
#endif

namespace gcore{
  class System;
}

namespace gio{
  class OutPdb_i;
  
  class OutPdb: public OutCoordinates{
    OutPdb_i *d_this;
    // not implemented
    OutPdb(const OutPdb &);
    OutPdb &operator=(const OutPdb&);
  public:
    OutPdb();
    OutPdb(std::ostream &os);
    ~OutPdb();
    void select(const std::string &thing);
    void open(std::ostream &os);
    void close();
    void writeTitle(const std::string &title);
    OutPdb &operator<<(const gcore::System &sys);
  };
}

#endif
