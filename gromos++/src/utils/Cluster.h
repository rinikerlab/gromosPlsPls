// utils_Cluster.h


#ifndef INCLUDED_UTILS_CLUSTER
#define INCLUDED_UTILS_CLUSTER

#ifndef INCLUDED_VECTOR
#include <vector>
#define INCLUDED_VECTOR
#endif

namespace utils{

  class Cluster{
    public:
      int center;
      bool is_taken;
      vector<int> neighbors;
      Cluster::Cluster(): is_taken(false) {};
  };
}
#endif