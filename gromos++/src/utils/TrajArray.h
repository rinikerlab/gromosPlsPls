// TrajArray.h
#ifndef TRAJARRAY_H
#define TRAJARRAY_H
#include "../gcore/Box.h"
#include "../gcore/Molecule.h"
#include "../gcore/System.h"

namespace utils{
class TrajArray {

  public:
    // Constructor
    TrajArray(const gcore::System &sys);

    // Destructor
    ~TrajArray();

    // Function to store a frame
    void store(const gcore::System &sys,
      const unsigned int frameIndex);

    // Function to extract a frame
    void extract(gcore::System &sys,
      const unsigned int frameIndex) const;

    // Accessor for framesize
    inline unsigned int numAtoms();

  protected:
    vector<double *> trajectoryData;
    // number of coordinates per frame
    unsigned int nAtoms;

};
}
#endif                                                                 