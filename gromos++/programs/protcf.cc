// Distribution. Read in a set of numbers and output the distribution

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <set>

#include "../src/args/Arguments.h"
#include "../src/gmath/Stat.h"
#include "../src/gmath/Expression.h"
#include "../src/gmath/Distribution.h"
#include "../src/gmath/Correlation.h"
#include "../src/gmath/Vec.h"

using namespace args;
using namespace std;


int main(int argc, char **argv){

  char *knowns[] = {"files", "distribution", "normalize", "bounds","tcf", 
		    "expression", "spectrum", "substract_average", "time"};
  int nknowns = 9;

  string usage = argv[0];
  usage += "\n\t@files         <data file>\n";
  usage += "\t@time          <time> <time step>\n";
  usage += "\t[@distribution <data columns to consider>]\n";
  usage += "\t  [@bounds     <lower bound> <upper bound> <grid points>]\n";
  usage += "\t  [@normalize]\n";
  usage += "\t[@tcf          <data columns to consider>]\n";
  usage += "\t  [@expression <expression for correlation function>]\n";
  usage += "\t  [@spectrum   <noise level>]\n";
  usage += "\t  [@substract_average]\n";

  try{
    Arguments args(argc, argv, nknowns, knowns, usage);
    
    // read the time
    double time=0, dt=1;
    {
      Arguments::const_iterator iter=args.lower_bound("time");
      if(iter!=args.upper_bound("time")){
        time=atof(iter->second.c_str());
        ++iter;
      }
      if(iter!=args.upper_bound("time"))
        dt=atof(iter->second.c_str());
    }

    // determine which data to store
    set<int> data_sets;
    vector<int> data_index;
    
    // get all input for distributions
    bool distribution = false;
    double dist_lower = 0.0;
    double dist_upper = 1.0;
    int dist_grid = 10;
    bool dist_normalize = false;
    vector<int> dist_index;
    for(Arguments::const_iterator iter=args.lower_bound("distribution"), 
	  to=args.upper_bound("distribution");
	iter!=to; ++iter){
      int i=atoi(iter->second.c_str())-1;
      dist_index.push_back(i);
      data_sets.insert(i);
      distribution=true;
    }
    if(args.count("normalize")>=0) dist_normalize=true;
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

    bool tcf=false;
    bool tcf_vector=false;
    bool tcf_expression=false;
    bool tcf_spectrum = false;
    bool tcf_substract = false;
    bool tcf_auto = false;
    
    string tcf_expression_string;
    double tcf_noise = 1.0;
    vector<int> tcf_index;
    for(Arguments::const_iterator iter=args.lower_bound("tcf"), 
	  to=args.upper_bound("tcf");
	iter!=to; ++iter){
      int i=atoi(iter->second.c_str())-1;
      tcf_index.push_back(i);
      data_sets.insert(i);
      tcf=true;
    }

    // handle the cases for autocorrelation functions
    // and vector correlation functions
    if(tcf){
      if(tcf_index.size()==1) {
	tcf_index.push_back(tcf_index[0]);
	tcf_auto=true;
      }
      else if(tcf_index.size()==3){
	tcf_index.push_back(tcf_index[0]);
	tcf_index.push_back(tcf_index[1]);
	tcf_index.push_back(tcf_index[2]);
	tcf_vector=true;
	tcf_auto=true;
      }
      else if(tcf_index.size()==6) tcf_vector=true;
      else if(tcf_index.size()!=2)
	throw gromos::Exception("protcf",
				"Specify either 1, 2, 3, or 6 columns for tcf");
    }
    
    if(args.count("expression")>1){
      tcf_expression = true;
      for(Arguments::const_iterator iter=args.lower_bound("expression"),
	    to =args.upper_bound("expression"); iter!=to; ++iter){
	tcf_expression_string += iter->second + " ";
      }
    }
    if(args.count("substract_average")>=0) tcf_substract=true;
    if(args.count("spectrum")>=0){
      tcf_spectrum=true;
      if(args.count("spectrum")==1) tcf_noise=atof(args["spectrum"].c_str());
    }
    
    // process the data sets that need to be stored.
    int data_max=0;
    for(set<int>::const_iterator iter=data_sets.begin(), 
	  to=data_sets.end(); 
	iter!=to; ++iter){
      
      bool keep=true;
      if(tcf_vector){
	keep=false;
	for(unsigned int i=0; i<dist_index.size(); i++)
	  if(*iter==dist_index[i]) keep=true;
      }
      if(keep)
	data_index.push_back(*iter);
      if(*iter>data_max) data_max=*iter;
    }
    data_max++;
    
    // calculate sizes for datastructures
    int data_num = data_index.size();
    
    // we will also need the inverted data_index
    vector<int> data_inv(data_max);
    for(int i=0; i< data_num; i++)
      data_inv[data_index[i]]=i;
    
    // create data structures to read data into
    vector<gmath::stat> data;
    for(int i=0; i<data_num; i++){
      gmath::stat tmp;
      data.push_back(tmp);
    }
    
    vector<vector<gmath::Vec> > data_vec(2);
    vector<double> tmp_data(data_max);
    
    if(args.count("files")<=0)
      throw gromos::Exception("protcf", "There is no data file specified\n"+
			      usage);
    
    // ALL INPUT GATHERED:
    // loop over the files and store data
    string line;
    stringstream linestream;
    for(Arguments::const_iterator 
        iter=args.lower_bound("files"),
        to=args.upper_bound("files");
      iter!=to; ++iter){
      
      ifstream file(iter->second.c_str());

      do{
	getline(file, line, '\n');
	if(!file.eof() && line[0]!='#'){
	  linestream.clear();
	  linestream.str(line);
	  for(int i=0; i<data_max; i++)
	    linestream >> tmp_data[i];
	  if(!linestream.good() && !linestream.eof()){
	    ostringstream os;
	    os << "failed to read " << data_max << " values from line\n"
	       << line << "\ngot\n";
	    for(int i=0; i<data_max; i++)
	      os << tmp_data[i] << "  ";
	    throw gromos::Exception("protcf", os.str());
	  }
	  
	  for(unsigned int j=0; j < data_index.size(); j++)
	    data[j].addval(tmp_data[data_index[j]]);
	  if(tcf_vector){
	    gmath::Vec v1(tmp_data[tcf_index[0]],
			  tmp_data[tcf_index[1]],
			  tmp_data[tcf_index[2]]);
	    data_vec[0].push_back(v1);
	    gmath::Vec v2(tmp_data[tcf_index[3]],
			  tmp_data[tcf_index[4]],
			  tmp_data[tcf_index[5]]);
	    data_vec[1].push_back(v1);
	  }
	}
      } while(!file.eof());
      
	
      file.close();
    }
    
    // OK, we have all the data, now we need to do whatever we are expected
    // to do

    // CALCULATIONS AND OUTPUT
    cout << "TITLE\n"
	 << "Statistical analysis of data file";
    if(args.count("files")>1) cout << "s";
    cout << ":\n";
    for(Arguments::const_iterator 
        iter=args.lower_bound("files"),
        to=args.upper_bound("files");
      iter!=to; ++iter){
      cout << iter->second << "\n";
    }
    cout << "END\n";
    
    // averages etc.
    if(data_index.size()){
      cout << "STATISTICS\n"
	   << "# column       N     average        rmsd  error est."
	   << "     minimum     maximum\n";
      for(unsigned int i=0; i< data_index.size(); i++){
	int di=data_inv[data_index[i]];
	cout << setw(8) << data_index[i]+1
	     << setw(8) << data[di].n()
	     << setw(12) << data[di].ave()
	     << setw(12) << data[di].rmsd()
	     << setw(12) << data[di].ee()
	     << setw(12) << data[di].min()
	     << setw(12) << data[di].max()
	     << "\n";
      }
      cout << "END\n";
    }
    
    // create the distributions
    if(distribution){
      cout << "DISTRIBUTION\n"
	   << "# distributions calculated for column";
      if(dist_index.size()>1) cout << "s";
      for(unsigned int i=0; i< dist_index.size(); i++){
	int di=data_inv[dist_index[i]];
	data[di].dist_init(dist_lower, dist_upper, dist_grid);
	cout << " " << dist_index[i]+1;
      }
      cout << "\n# lower bound: " << dist_lower
	   << "\n# upper bound: " << dist_upper
	   << "\n# number of grid points: " << dist_grid
	   << "\n# distribution is ";
      if(!dist_normalize) cout << "not ";
      cout << "normalized\n\n"
	   << "#    value";
      for(unsigned int i=0; i< dist_index.size(); i++)
	cout << setw(6) << dist_index[i]+1 << ". column";
      cout << "\n";
      
      for(int i=0; i < dist_grid; i++){
	int dd=data_inv[dist_index[0]];
	cout << setw(10) << data[dd].distribution()->value(i);
	for(unsigned int j=0; j < dist_index.size(); j++){
	  int dj=data_inv[dist_index[j]];
	  if(dist_normalize)
	    cout << setw(14) << double((*data[dj].distribution())[i])/
	      data[dj].distribution()->nVal();
	  else
	    cout << setw(14) << (*data[dj].distribution())[i];
	}
	cout << "\n";
      }
      cout << "END\n";
    }
    if(tcf){
      cout << "TIME CORRELATION FUNCTION\n"
	   << "# calculating ";
      if(tcf_auto) cout << "auto-";
      cout << "correlation function for ";
      if(tcf_vector) {
	cout << "vector";
	if(!tcf_auto) cout << "s";
	cout << " defined by ";
      }
      cout << "column";
      if(!tcf_auto || tcf_vector) cout << "s";
      int limit=tcf_index.size();
      if(tcf_auto) limit/=2;
      for(int i=0; i<limit; i++){
	if(i==int(tcf_index.size())/2) cout << " (A) and";
	cout << " " << tcf_index[i]+1;
      }
      if(!tcf_auto) cout << " (B)";
      cout << "\n\n";
      if(tcf_substract)
	cout << "# average values are substracted from time series\n\n";
      cout << "# correlation function calculated as C(t) = <";
      if(tcf_expression){
	cout << " f( A(T), ";
	if(tcf_auto) cout << "A"; else cout << "B";
	cout << "(T+t) ) ";
      }
      else{
	cout << " A(T) * ";
	if(tcf_auto) cout << "A"; else cout << "B";
	cout << "(T+t) ";
      }
      cout << ">_T\n";
      if(tcf_expression){
	cout << "# with f( A(T), ";
	if(tcf_auto) cout << "A"; else cout << "B";
	cout << "(T+t) ) = " << tcf_expression_string << "\n";
      }
      cout << "# using ";
      if(tcf_expression || tcf_vector)
	cout << "a double loop algorithm\n";
      else
	cout << "fast fourier transforms\n";
      
      gmath::correlation *corr;
      // several cases
      if(tcf_vector){
	corr = new gmath::correlation(data_vec[0],data_vec[1]);
	corr->calc_direct();
      }
      else{
	int d1=data_inv[tcf_index[0]];
	int d2=data_inv[tcf_index[1]];
	if(tcf_substract){
	  data[d1].substract_average();
	  data[d2].substract_average();
	}
	corr = new gmath::correlation(data[d1], data[d2]);
	if(tcf_expression){
	  corr->calc_expression(tcf_expression_string);
	}
	
	else{
	  corr->calc_fft();
	}
	
      }
      cout << "\n#        t          C(t)\n";
      
      for(int i=0; i<corr->size(); i++, time+=dt){
	cout << setw(10) << time 
	     << setw(14) << (*corr)[i] << "\n";
      }
      cout << "END\n";
      
      if(tcf_spectrum){
	cout << "SPECTRUM\n"
	     << "# calculated from above correlation function\n"
	     << "# " << tcf_noise *100.0 << " % of C(t) used in spectrum "
	     << "calculation\n\n"
	     << "#  frequency   intensity\n";
	
	vector<double> freq, spec;
	corr->spectrum(freq, spec, dt, tcf_noise);
	for(unsigned int i=0; i<freq.size(); i++){
	  cout << setw(12) << freq[i] << setw(12) << spec[i] << "\n";
	}
	cout << "END\n";
      }
      
    }
    
    
  }
  
  catch (const gromos::Exception &e){
    cerr << e.what() << endl;
    exit(1);
  }
  return 0;
}



