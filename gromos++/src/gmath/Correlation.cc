// gmath_correlation.cc

#include "Correlation.h"
#include "Stat.h"
#include "Expression.h"
#include "physics.h"
#include "../gromos/Exception.h"
#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_fft_halfcomplex.h>
#include <vector>
#include <cmath>
#include <new>

using namespace gmath;

namespace gmath
{
  correlation::correlation(std::vector<double>& a, std::vector<double>& b){
    if(a.size() != b.size())
      throw(gromos::Exception("Correlation", "Specified data vectors do not have the same length!"));

    d_a=&a;
    d_b=&b;
    d_f.resize(d_a->size(), 0.0);
    d_vec=false;
    d_calc=false;
  }

  correlation::correlation(gmath::stat &a, gmath::stat &b){
    if(a.n() != b.n())
      throw(gromos::Exception("Correlation", "Specified data sets do not have the same number of elements!"));
    d_a=a.data();
    d_b=b.data();
    d_f.resize(d_a->size(), 0.0);
    d_vec=false;
    d_calc=false;
  }

  correlation::correlation(std::vector<gmath::Vec>& a, 
			   std::vector<gmath::Vec>& b){
    if(a.size() != b.size())
      throw(gromos::Exception("Correlation", "Specified data sets do not have the same number of elements!"));
    d_va = &a;
    d_vb= &b;
    d_vec=true;
    d_f.resize(d_va->size());
    d_calc=false;
  }

  void correlation::calc_direct()
  {
    int num=d_f.size();
    for(int i=0; i< num; i++) d_f[i]=0.0;
    if(d_vec){
      for(int i=0; i<num; i++){
        for(int j=i+1; j<num; j++){
          d_f[j-i]+=(*d_va)[i].dot((*d_vb)[j]);
        }
      }
    }
    else{
      for(int i=0; i<num; i++){
        for(int j=i+1; j<num; j++){
          d_f[j-i]+=(*d_a)[i] * (*d_b)[j];
        }
      }
    }
    for(int i=1; i<num; i++){
      d_f[i]/=(num-i);
    }
    d_calc=true;
  }

  void correlation::calc_fft(){
    // unfortunately, we have to copy the data to arrays of doubles
    // in order to be able to calculate the fourier transforms with the gsl
    // and in order to prevent spurious oscillations, we fill double the data
// size
    if(d_vec)
      throw(gromos::Exception("Correlation", "calculation of correlation function with ffts currently not possible for dot products of vectors"));
    int num=d_a->size();
    double a[2*num];
    double b[2*num];
    double c[2*num];
    for(int i=0; i< num; i++){
	a[i]=(*d_a)[i];
	a[2*num-i-1]=0.0;
	b[i]=(*d_b)[i];
	b[2*num-i-1]=0.0;
    }
    // now take the fourier transform
    gsl_fft_real_wavetable * real;
    gsl_fft_halfcomplex_wavetable * hc;
    gsl_fft_real_workspace * work;
    
    work = gsl_fft_real_workspace_alloc (2*num);
    real = gsl_fft_real_wavetable_alloc (2*num);
    hc = gsl_fft_halfcomplex_wavetable_alloc (2*num);
    
    gsl_fft_real_transform (a, 1, 2*num, real, work);
    gsl_fft_real_transform (b, 1, 2*num, real, work);

    // the fourier transform of the correlation function is the
    // product of a and b. Unfortunately these are complex numbers
    c[0]=a[0]*b[0];
    for(int i=1; i<2*num; i+=2){
      c[i] = a[i]*b[i] + a[i+1]*b[i+1];
      c[i+1] = a[i]*b[i+1] - a[i+1]*b[i];
    }
    // now take the inverse fourier transform of c and store the values in 
    // d_f
    gsl_fft_halfcomplex_inverse (c, 1, 2*num, hc, work);
    
    for(int i=0; i<num; i++){
      d_f[i]=c[i]/(num-i);
    }
    gsl_fft_real_wavetable_free (real);
    gsl_fft_halfcomplex_wavetable_free (hc);
    gsl_fft_real_workspace_free(work);
    d_calc=true;
  }

  void correlation::calc_expression(std::string s){
      // first find the words "A" and "B"
      std::string::size_type iter=s.find("A", 0);
      while (iter!=std::string::npos) {
	  s.replace(iter,1,"a1",2);
	  iter=s.find("A",iter);
      }
      iter=s.find("B", 0);
      while(iter!=std::string::npos){
          s.replace(iter, 1, "a2", 2);
	  iter=s.find("B", iter);
      }

      // now we can set the expression
      gmath::Expression e(s);
    
      // prepare a vector to set the values
      std::vector<double> v(2);

      // do a (direct) calculation
      int num=d_f.size();
      for(int i=0; i<num; i++) d_f[i]=0.0;
      for(int i=0; i<num; i++){
	v[0]=(*d_a)[i];
        for(int j=i+1; j<num; j++){
	  v[1]=(*d_b)[j];
	  e.setValues(v);
	  d_f[j-i]+=e.value();
        }
      }
      for(int i=1; i<num; i++){
	d_f[i]/=(num-i);
      }
      d_calc=true;
  }

  double correlation::operator[](int i){
    assert(i < int(d_f.size()));
    return d_f[i];
  }
  int correlation::size(){
    return d_f.size();
  }

  void correlation::spectrum(std::vector<double>& w, std::vector<double>& s, 
			     double dt, double frac)
  {
    int num=int(frac*d_a->size());
    if(!d_calc) throw gromos::Exception("correlation",
		"calculate the correlation function before the spectrum");
    
    /*
     * Calculation of the power spectrum is currently carried out accoring
     * to the following steps:
     * 1. Calculate the (auto)correlation function
     * 2. Smoothen the (auto)correlation function with a cosine, to make
     *    sure it goes to zero at the end of the function
     * 3. Mirror the resulting function, this will yield a periodic function
     *    of 2*num elements with a value of zero in the middle
     * 4. Take the fourier transform of this function.
     *
     * Options that are currently not implemented but could be relevant:
     * a. Take only the first halve of the correlation function: These 
     *    functions are often noisy at longer times cutting it off at 
     *    num/2 ensures that we only take points with sufficient statistical
     *    significance. Due to the doubling of the data (mirroring) we would
     *    still get the same number of data points in the spectrum
     *    Interestingly enough, the resulting spectrum has quite a lot less
     *    noise. It corresponds quite exactly to the spectrum calculated from 
     *    the complete correlation function with a running average over 4 data
     *    points applied to the spectrum. Is there an analytical reason for 
     *    this (4 = 2 x 2) ?
     * b. Multiplying the final spectrum with the frequency squared. Depending
     *    on the definition of the power-spectrum that one uses, this might
     *    be required. (Autocorrelation function of the velocity?)
     * c. Herman also added an additional division by the number of data
     *    points. Possibly also from the definition of the spectrum?
     */

    // we have to translate to the gsl standards this means copying a lot
    // of data -- who cares
    double data[2*num];
    double dw=0.5/dt/num;

    w.resize(num,0.0);
    s.resize(num,0.0);
    

    // copy the correlation function over to data and smoothen by a cosine 
    // function: 0.5*(1+cos(pi*i/num)
    double factor;
    
    for(int i=0; i<num; i++){
      factor=0.5*(1.0+cos(M_PI*i/num));
      data[i]=d_f[i] * factor;
    }
    
    // and mirror this in the last num elements
    for(int i=0; i<num; i++)
      data[2*num-i-1]=data[i];

    // now take the fourier transform
    gsl_fft_real_wavetable * real;
    gsl_fft_real_workspace * work;
    
    work = gsl_fft_real_workspace_alloc (2*num);
    real = gsl_fft_real_wavetable_alloc (2*num);
    
    gsl_fft_real_transform (data, 1, 2*num, real, work);
    
    gsl_fft_real_wavetable_free (real);
    gsl_fft_real_workspace_free (work);
    // the real space elements are in the elements 0, 1, 3, 5... of data
    // put those in the vector elements and modify

    int k;
    s[0]=data[0];
    for(int i=1; i<num; i++){
      k=2*i-1;
      // not sure about the i*i/num
      s[i]=data[k];
      w[i]=i*dw;
    }
  }
}









