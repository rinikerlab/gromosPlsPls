/**
 * @file reweight.cc
 * reweight a time series
 */

/**
 * @page programs Program Documentation
 *
 * @anchor reweight
 * @section reweight reweight a time series
 * @author @ref cc
 * @date 04. 06. 09
 *
 * Reweights a time series of observed values of @f$X@f$ sampled during a simulation
 * at state @f$R@f$ (i.e. using the Hamiltonian @f$H_R=K_R(\vec{p})+V_R(\vec{r})@f$)
 * to another state @f$Y@f$ (neglecting kinetic contributions for simplicity):
 * @f[ \langle X \rangle_Y =
 *    \frac
 *        {\langle X \exp \left[-\beta \left (V_Y - V_R \right) \right] \rangle_R}
 *        {\langle   \exp \left[-\beta \left (V_Y - V_R \right) \right] \rangle_R}
 * = \langle X \exp \left[-\beta \left (V_Y - V_R -\Delta F_{YR} \right) \right] \rangle_R
 * @f]
 * with @f$\Delta F_{YR}=F_Y - F_R@f$.
 * The observed quantitiy @f$X@f$ can be a structural quantity (e.g. the time series
 * of an angle) or an energetic quantity (e.g. the time series of the ligand-protein
 * interaction energy). Note that the reweighting will only give useful results
 * if during the simulation at state @f$R@f$ all configurations that are important
 * to @f$Y@f$ are sampled.
 * The program reads three time series corresponding to the quantitiy @f$X@f$,
 * the energy of state @f$R@f$, and the energy of state @f$Y@f$. All time series
 * must have been calculated from the same ensemble @f$R@f$. The time series
 * files consist of a time column and a column containing the quantity (i.e. @f$X@f$,
 * @f$V_R@f$, or @f$V_Y@f$). The time series are obtained e.g. by @ref ene_ana or @ref tser.
 * If the bounds flag is given a normalized distribution of @f$X@f$ in the @f$Y@f$ ensemble will be
 * written out.
 * When calculating averages and distributions special care is taken
 * in order to avoid overflow (see Comput. Phys. Comm. 2003, 153, 397-406).
 *
 * <b>arguments:</b>
 * <table border=0 cellpadding=0>
 * <tr><td>  \@temp</td><td>&lt;temperature for perturbation&gt; </td></tr>
 * <tr><td>  \@x</td><td>&lt;time series of quantity X&gt; </td></tr>
 * <tr><td>  \@vr</td><td>&lt;energy time series of state R&gt; </td></tr>
 * <tr><td>  \@vy</td><td>&lt;energy time series of state Y&gt; </td></tr>
 * <tr><td> [\@bounds</td><td>&lt;lower bound&gt; &lt;upper bound&gt; &lt;grid points&gt;] </td></tr>
 * </table>
 *
 * Example:
 * @verbatim
 reweight
    @temp      300
    @x         tser.dat
    @vr        eR.dat
    @vy        eY.dat
    @bounds    -300 300 100
    @endverbatim
 *
 * <hr>
 */

#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <sstream>

#include "../src/args/Arguments.h"
#include "../src/gmath/WDistribution.h"
#include "../src/gmath/Expression.h"
#include "../src/gmath/Physics.h"

using namespace std;
using namespace args;

gmath::Stat<double> read_data(string name, Arguments &args);

int main(int argc, char** argv) {
  
  Argument_List knowns;

  knowns << "temp" << "x" << "vr" << "vy" << "bounds";

  string usage = "# " + string(argv[0]);
  usage += "\n\t@temp     <temperature for perturbation>\n";
  usage +=   "\t@x        <time series of quantity X>\n";
  usage +=   "\t@vr       <energy time series of state R>\n";
  usage +=   "\t@vy       <energy time series of state Y>\n";
  usage +=   "\t[@bounds  <lower bound> <upper bound> <grid points>]\n";
 
  try {
    
    Arguments args(argc, argv, knowns, usage);

    // Get temperature as a double
    args.check("temp",1);
    double temp;
    {
      std::istringstream is(args["temp"]);
      if (!(is >> temp)) {
        throw gromos::Exception(argv[0],
                "temperature not numeric");
      }
    }

    // read the bounds if given
    double dist_lower = 0.0;
    double dist_upper = 1.0;
    int dist_grid = 10;
    {
      Arguments::const_iterator iter=args.lower_bound("bounds");
      if(iter!=args.upper_bound("bounds")){
        dist_lower=atof(iter->second.c_str());
        ++iter;
      }
      if(iter!=args.upper_bound("bounds")){
        dist_upper=atof(iter->second.c_str());
        ++iter;
      }
      if(iter!=args.upper_bound("bounds"))
        dist_grid=atoi(iter->second.c_str());
    }

    // read the time series
    gmath::Stat<double> x = read_data("x", args);
    gmath::Stat<double> vr = read_data("vr", args);
    gmath::Stat<double> vy =read_data("vy", args);

    // check whether all time series have the same length
    if ( x.n()!=vr.n() || x.n()!=vr.n()  )
      throw gromos::Exception("reweight", "Time series files differ in length!\n");

    // save -beta(V_Y - V_R)
    gmath::Stat<double> vyvr;
    // create a distribution (with weights != 1)
    gmath::WDistribution xexpvyvr(dist_lower,dist_upper,dist_grid);
     
    /* loop over data that has been read in.
     * If speed turns out to be an issue this can be done
     * also on the fly when reading in the data
     */
    for (int i = 0; i < vr.n(); i++) {
      double diff = -(vy.data()[i] - vr.data()[i]) / (gmath::boltz * temp);
      vyvr.addval(diff);
      xexpvyvr.add(x.data()[i],diff);
    }

    cout.precision(10);
    // Calculate ln{|<X*exp[-beta(V_Y - V_R)]>_R|}
    int sign = 0;
    double lnXexpave = gmath::Stat<double>::lnXexpave(x, vyvr, sign);
    cout << "# ln{|<X*exp[-beta(V_Y - V_R)]>_R|} = " << lnXexpave << endl;
    cout << "# sign = " << sign << endl;
    // Calculate ln{<exp[-beta(V_Y - V_R)]>_R}
    cout << "# ln{<exp[-beta(V_Y - V_R)]>_R} = " << vyvr.lnexpave() << endl;
    // <X>_Y
    cout << "# <X>_Y = " << exp(lnXexpave - vyvr.lnexpave()) * sign << endl;

    // Write out a distribution if the @bounds flag is given
    if (args.count("bounds") >= 0) {
      // write a normalized distribution
      xexpvyvr.write_normalized(cout);
      //xexpvyvr.write(cout);
    }
  
  } catch (const gromos::Exception &e) {
    cerr << e.what() << endl;
    exit(1);
  }
   
  return 0;
}

gmath::Stat<double> read_data(string name, Arguments & args) {

  gmath::Stat<double> data;

  // make sure we have the @name flag
  args.check(name.c_str(), 1);

  // open the time series file for quantity x
  ifstream x;
  double time, q;
  Arguments::const_iterator iter2 = args.lower_bound(name.c_str());

  x.open((iter2->second).c_str());
  if (!x)
    throw gromos::Exception("reweight", "Could not open time series file for " + name + ".\n");

  // read
  string sdum;
  while (true) {
    std::getline(x, sdum);
    if (x.eof()) {
      break;
    }
    std::string::size_type it = sdum.find('#');
    if (it != std::string::npos)
      sdum = sdum.substr(0, it);
    if (sdum.find_first_not_of(" \t") == std::string::npos)
      continue;
    std::istringstream is(sdum);
    if ((!(is >> time >> q)) && !x.eof())
      throw gromos::Exception("reweight", "Error when reading from " + name + " time series file.\n");
    data.addval(q);
    
  }
  return data;
}