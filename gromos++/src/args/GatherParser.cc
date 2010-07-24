// args_GatherParser.cc

#include "GatherParser.h"
#include "Arguments.h"
#include "../bound/Boundary.h"

using namespace args;
using namespace bound;

bound::Boundary::MemPtr GatherParser::parse(const Arguments &args, const std::string &str) {
  Boundary::MemPtr gathmethod;

  try {

    Arguments::const_iterator it = args.lower_bound(str);
    if (it == args.upper_bound(str))
      throw Arguments::Exception("");
    ++it;

    if (it == args.upper_bound(str)) {
      gathmethod = &Boundary::coggather;
    } else {
      std::string gather = it->second;

      if (gather == "nog") {
        gathmethod = &Boundary::nogather;
      } else if (gather == "g") {
        gathmethod = &Boundary::gather;
      } else if (gather == "ggr") {
        gathmethod = &Boundary::gathergr;
      } else if (gather == "mgr") {
        gathmethod = &Boundary::gathermgr;
      } else if (gather == "cog") {
        gathmethod = &Boundary::coggather;
      } else if (gather == "crs") {
        gathmethod = &Boundary::crsgather;
      } else if (gather == "seq") {
        gathmethod = &Boundary::seqgather;
      } else if (gather == "gen") {
        gathmethod = &Boundary::gengather;
      } else if (gather == "bg") {
        gathmethod = &Boundary::bondgather;
      } else if (gather == "refg") {
        gathmethod = &Boundary::refgather;
      } else if (gather == "1") {
        gathmethod = &Boundary::gatherlist;
      } else if (gather == "2") {
        gathmethod = &Boundary::gathertime;
      } else if (gather == "3") {
        gathmethod = &Boundary::gatherref;
      } else if (gather == "4") {
        gathmethod = &Boundary::gatherltime;
      } else if (gather == "5") {
        gathmethod = &Boundary::gatherrtime;
      } else if (gather == "6") {
        gathmethod = &Boundary::gatherbond;
      } else {
        throw gromos::Exception("Gather", gather +
                " unknown. Known gathering methods are nog, g, ggr, cog, crs, seq, gen, bg");
      }
    }
  } catch (Arguments::Exception &e) {
    //gathmethod = &Boundary::coggather;
      gathmethod = &Boundary::gatherlist;
  }


  return gathmethod;
}
