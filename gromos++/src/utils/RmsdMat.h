// utils_RmsdMat.h

#ifndef INCLUDED_UTILS_RMSDMAT
#define INCLUDED_UTILS_RMSDMAT

#include <vector.h>

namespace utils{

  class RmsdMat{
    public:

      // Constructor
      RmsdMat(const unsigned int n);

      // Destructor
      ~RmsdMat();

      void insert(const unsigned int i, 
        const unsigned int j, const float rmsd);
      float retrieve(const unsigned int i, 
        const unsigned int j) const;
      unsigned int size_n() const;

    protected:
      unsigned int index(const unsigned int i,
        const unsigned int j) const;
      const unsigned int width;
      vector<float> matrix;
  };
}

#endif