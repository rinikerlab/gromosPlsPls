/*
 * This file is part of GROMOS.
 * 
 * Copyright (c) 2011, 2012, 2016, 2018, 2021, 2023 Biomos b.v.
 * See <https://www.gromos.net> for details.
 * 
 * GROMOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <cstdlib>
#include <vector>
#include <set>
#include "LJException.h"
#include "MoleculeTopology.h"
#include "AtomTopology.h"
#include "Bond.h"
#include "Angle.h"

using namespace gcore;
using namespace std;

int main() {
  MoleculeTopology mt;
  AtomTopology at;
  at.setIac(3);
  mt.addAtom(at);
  at.setIac(4);
  mt.addAtom(at);

  cout << "AtomTopology IAC: ";
  for (int i = 0; i < mt.numAtoms(); ++i)
    cout << mt.atom(i).iac() << ' ';
  cout << endl;

  Bond b(1, 2);
  mt.addBond(b);
  b = Bond(2, 3);
  mt.addBond(b);

  cout << "Bonds: ";
  BondIterator bi(mt);
  for (; bi; ++bi)
    cout << '(' << bi()[0] << ',' << bi()[1] << ") ";
  cout << endl;

  Angle ang(1, 2, 3);
  mt.addAngle(ang);
  ang = Angle(4, 5, 6);
  mt.addAngle(ang);

  cout << "Angles: ";
  AngleIterator ai(mt);
  for (; ai; ++ai)
    cout << '(' << ai()[0] << ' ' << ai()[1] << ' ' << ai()[2] << ") ";
  cout << endl;

  mt.setResName(4, "ASP");
  cout << "Resnames: ";
  for (int i = 0; i < 5; ++i)
    cout << i << ' ' << mt.resName(i) << ' ';
  cout << endl;

  return 0;
}
