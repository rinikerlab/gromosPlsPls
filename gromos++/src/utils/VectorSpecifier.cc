/**
 * @file VectorSpecifier.cc
 * vector specifier implementation
 */
#include <cassert>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>

#include "../bound/Boundary.h"
#include "VectorSpecifier.h"

using namespace gcore;

utils::VectorSpecifier::VectorSpecifier(gcore::System &sys, bound::Boundary * pbc, std::string s)
  : d_atomspec(sys),
    d_pbc(pbc)
{
  parse(s);
}

utils::VectorSpecifier::VectorSpecifier(utils::VectorSpecifier const & vs)
  : d_atomspec(vs.d_atomspec),
    d_vec(vs.d_vec),
    d_pbc(vs.d_pbc)
{
}


utils::VectorSpecifier::~VectorSpecifier()
{
}

int utils::VectorSpecifier::setSpecifier(std::string s)
{
  clear();
  parse(s);
  return 0;
}

utils::VectorSpecifier & utils::VectorSpecifier::operator=
(
 utils::VectorSpecifier const & vs
 )
{
  d_atomspec = vs.d_atomspec;
  d_vec = vs.d_vec;
  return *this;
}

gmath::Vec const & utils::VectorSpecifier::operator()()
{
  if (d_atomspec.size() > 1)
    d_vec = d_atomspec.pos(0) - d_pbc->nearestImage(d_atomspec.pos(0),d_atomspec.pos(1), 
						    d_atomspec.sys()->box());
  else if (d_atomspec.size() == 1)
    d_vec = d_atomspec.pos(0);
  return d_vec;
}

void utils::VectorSpecifier::clear()
{
  d_vec = 0.0;
  d_atomspec.clear();
}

gcore::System & utils::VectorSpecifier::sys()
{
  return *d_atomspec.sys();
}

bound::Boundary * utils::VectorSpecifier::pbc()
{
  return d_pbc;
}

std::string utils::VectorSpecifier::toString()
{
  std::ostringstream os;
  if (d_atomspec.size()){
    os << "atom("  << d_atomspec.toString()[0] << ")";
  }
  else{
    os << "cart(" << d_vec[0]
       << "," << d_vec[1]
       << "," << d_vec[2]
       << ")";
  }
  return os.str();
}

void utils::VectorSpecifier::parse(std::string s)
{
  std::string::size_type bra = s.find('(');
  if (bra == std::string::npos)
    throw Exception("wrong format (no ())");
  std::string::size_type ket = find_matching_bracket(s, '(', bra+1);
  if (ket == std::string::npos)
    throw Exception("no closing bracket found!");
  
  std::string b = s.substr(0, bra);
  std::string rest = s.substr(bra+1, ket - bra - 2);
  
  if (b == "cart"){
    parse_cartesian(rest);
  }
  else if (b == "polar"){
    parse_polar(rest);
  }
  else if (b == "atom"){
    parse_atom(rest);
  }
  else{
    throw Exception("wrong format : type " + b);
  }
}

void utils::VectorSpecifier::parse_atom(std::string s)
{
  // std::cout << "parse_atom : " << s << "..." << std::endl;
  d_atomspec.addSpecifier(s);
}

void utils::VectorSpecifier::parse_cartesian(std::string s)
{
  // std::cout << "parse_cartesian : " << s << "..." << std::endl;

  std::string::size_type it = s.find(',');
  if (it == std::string::npos)
    throw Exception("cartesian: vector separated by , expected!");
  
  std::istringstream is(s.substr(0, it));
  if (!(is >> d_vec[0]))
    throw Exception("cartesian: could not read number");
  
  std::string s2 = s.substr(it+1, std::string::npos);

  it = s2.find(',');
  if (it == std::string::npos)
    throw Exception("cartesian: vector separated by , expected!");
  
  is.clear();
  is.str(s2.substr(0, it));
  if (!(is >> d_vec[1]))
    throw Exception("cartesian: could not read number");

  is.clear();
  is.str(s2.substr(it+1, std::string::npos));
  if (!(is >> d_vec[2]))
    throw Exception("cartesian: could not read number");
  
  // std::cout << "cartesian read: " << d_vec[0] << " | " 
  // << d_vec[1] << " | " << d_vec[2] << std::endl;

}


void utils::VectorSpecifier::parse_polar(std::string s)
{
  // std::cout << "parse_polar : " << s << "..." << std::endl;

  d_vec = 0.0;
  double alpha, beta, r;
  
  std::string::size_type it = s.find(',');
  if (it == std::string::npos)
    throw Exception("cartesian: vector separated by , expected!");
  
  std::istringstream is(s.substr(0, it));
  if (!(is >> r))
    throw Exception("cartesian: could not read number");
  
  std::string s2 = s.substr(it+1, std::string::npos);

  it = s2.find(',');
  if (it == std::string::npos)
    throw Exception("cartesian: vector separated by , expected!");
  
  is.clear();
  is.str(s2.substr(0, it));
  if (!(is >> alpha))
    throw Exception("cartesian: could not read number");

  is.clear();
  is.str(s2.substr(it+1, std::string::npos));
  if (!(is >> beta))
    throw Exception("cartesian: could not read number");

  const double cosa = cos(alpha * M_PI / 180.0);
  const double sina = sin(alpha * M_PI / 180.0);
  const double cosb = cos(beta  * M_PI / 180.0);
  const double sinb = sin(beta  * M_PI / 180.0);
  
  d_vec[0] = cosa * cosb * r;
  d_vec[1] = sina * r;
  d_vec[2] = -sinb * cosa * r;

  // std::cout << "vector from polar: " << gmath::v2s(d_vec) << std::endl;

}

std::string::size_type utils::VectorSpecifier::find_matching_bracket
(
 std::string s,
 char bra,
 std::string::size_type it)
{
  char ket;
  if (bra == '(') ket = ')';
  else if (bra == '[') ket = ']';
  else if (bra == '{') ket = '}';
  else
    throw Exception("Bracket not recognised");
  
  int level = 1;
  for( ; it < s.length() && level != 0; ++it){
    if (s[it] == bra) ++level;
    else if (s[it] == ket) --level;
  }
  
  if (level) return std::string::npos;
  else return it;
}
