// Some functions needed by the program maketop
// several might be usefull for later programs as well

// the concept is that for modifying a topology, it is easier to store
// all atoms, bonds, angles etc in seperate vectors and then create the 
// topology out of that

// parseTopology returns a system based on these vectors, molecules are
// created based on the bonds
System parseTopology(vector<AtomTopology> *atoms, 
                     vector<Bond> *bonds, 
                     vector<Angle> *angles, 
                     vector<Improper> *imps, 
                     vector<Dihedral> *dihs, 
                     vector<string> *resNames, 
                     map<int, int> *resMap);

// the following set of functions are used to remove all atoms with iac < 0 
// from the vectors, exclusions and numberings are all adapted
void removeAtoms(vector <AtomTopology> *atoms,
                 vector <Bond> *bonds,
                 vector <Angle> *angles,
                 vector <Improper> *imps,
                 vector <Dihedral> *dihs);
void reduceAtoms(vector <AtomTopology> *atoms, set<int> rem, vector<int> ren);
void reduceBonds(vector <Bond> *bonds, set<int> rem, vector<int> ren);
void reduceAngles(vector <Angle> *angles, set<int> rem, vector<int> ren);
void reduceImps(vector <Improper> *imps, set<int> rem, vector<int> ren);
void reduceDihs(vector <Dihedral> *dihs, set<int> rem, vector<int> ren);

// a set of functions to add buildingblocks to the vectors. Look at the program 
// maketop for their intended use.
void addSolute(vector<AtomTopology> *atoms, 
               vector<Bond> *bonds,
               vector<Angle> *angles,
               vector<Improper> *imps,
               vector<Dihedral> *dihs,
               BbSolute bb, int rep);
int addBegin(vector<AtomTopology> *atoms, 
             BbEnd bb);
void addEnd(vector<AtomTopology> *atoms, 
            BbEnd bb);
void addCovEnd(vector<Bond> *bonds, 
               vector<Angle> *angles,
               vector<Improper> *imps,
               vector<Dihedral> *dihs,
               BbEnd bb, int offset);

// as a little extra, a function to generate all 1,4-interactions based on the
// bonds and an ugly hack to put in Cysteine bonds
void get14s(vector<AtomTopology> *atoms, vector<Bond> *bonds);
void setCysteines(vector<AtomTopology> *atoms,
		  vector<Bond> *bonds,
		  vector<Angle> *angles,
		  vector<Improper> *imps,
		  vector<Dihedral> *dihs,
		  int a, int b);

// ==========================================================================
//
// implementation:
//
// ===========================================================================

void addSolute(vector<AtomTopology> *atoms,
               vector<Bond> *bonds, 
	       vector<Angle> *angles,
               vector<Improper> *imps,
	       vector<Dihedral> *dihs,
	       BbSolute bb, int rep)
{
  int na=atoms->size();
  int strt=na-rep;
  int beg=0;
  if(strt<0) beg=-strt;

  for(int i=beg;i<rep;i++){
    Exclusion e;
    for(int j=0; j<bb.atom(i).exclusion().size(); j++)
      e.insert(bb.atom(i).exclusion().atom(j)+strt);
    (*atoms)[strt+i].setExclusion(e);
  }
  
  //or, if rep=0, but we have preceding exclusions
  if(rep==0){
    int pexl=strt-bb.numPexcl();
    if(pexl<0) throw gromos::Exception("addSolute", 
	       "Preceding exclusions, but no preceding atoms\n");
    
    for(int i=0; i<bb.numPexcl(); i++){
      Exclusion e;
      for(int j=0; j<bb.pexcl(i).size(); j++)
	e.insert(bb.pexcl(i).atom(j)+strt);
      (*atoms)[pexl+i].setExclusion(e);
    }
  }
  
  //finally, we add the atoms, but leave out the first rep
  for(int i=rep; i<bb.numAtoms();i++){
    Exclusion e;
    for(int j=0; j<bb.atom(i).exclusion().size(); j++)
      e.insert(bb.atom(i).exclusion().atom(j)+strt);
    
    atoms->push_back(bb.atom(i));
    (*atoms)[strt+i].setExclusion(e);
  }
  
  //now, bonded interactions
  int offset=strt;
  
  //bonds
  BbBondIt bi(bb);
  for(;bi;++bi){
    Bond b(bi()[0]+offset,bi()[1]+offset);
    b.setType(bi().type());

    //check if it exists already
    int found=0;
    if(rep>0)
      for(unsigned int k=0; k<bonds->size(); k++)
	if((*bonds)[k][0]==b[0]&&(*bonds)[k][1]==b[1]) found =1;
    
    if(!found&&b[0]>=0&&b[1]>=0)
      bonds->push_back(b);
  }
  
  //angles
  BbAngleIt ai(bb);
  for(;ai;++ai){
    Angle b(ai()[0]+offset,ai()[1]+offset, ai()[2]+offset);
    b.setType(ai().type());

    //check if it exists already
    int found=0;
    if(rep>0)
      for(unsigned int k=0; k<angles->size(); k++)
	if((*angles)[k][0]==b[0]&&(*angles)[k][1]==b[1]&&
	   (*angles)[k][2]==b[2]) found =1;

    if(b[0]>=na||b[1]>=na||b[2]>=na)
      if(!found&&b[0]>=0&&b[1]>=0&&b[2]>=0)
        angles->push_back(b);
  }
  //impropers
  BbImpIt ii(bb);
  for(;ii;++ii){
    Improper b(ii()[0]+offset,ii()[1]+offset, ii()[2]+offset, ii()[3]+offset);
    b.setType(ii().type());

    //check if it exists already
    int found=0;
    if(rep>0)
      for(unsigned int k=0; k<imps->size(); k++)
	if((*imps)[k][0]==b[0]&&(*imps)[k][1]==b[1]&&
	   (*imps)[k][2]==b[2]&&(*imps)[k][3]==b[3]) found =1;

    if(b[0]>=na||b[1]>=na||b[2]>=na||b[3]>=na)
      if(!found&&b[0]>=0&&b[1]>=0&&b[2]>=0&&b[3]>=0)
	imps->push_back(b);
  }
  
  //dihedrals
  BbDihIt di(bb);
  int last=dihs->size()-1;
  int counter=0;
  
  for(;di;++di){
    counter++;
    
    // find what is position -2 in the first dihedral
    int corr=offset;
    if(di()[0]==-3){
      for(unsigned int h=0; h<bonds->size(); h++)
	if((*bonds)[h][1]==di()[1]+offset) corr=(*bonds)[h][0];
      corr+=3;
    }
    // if rep!=0, do the same if di()[0]=-1;
    if(rep!=0&&di()[0]==-2){
      for(unsigned int h=0; h<bonds->size(); h++)
	if((*bonds)[h][1]==di()[1]+offset) corr=(*bonds)[h][0];
      corr+=2;
    }
    
    Dihedral b(di()[0]+corr,di()[1]+offset, di()[2]+offset, di()[3]+offset);
    b.setType(di().type());
    if(last>=0&&rep>0&&
       (*dihs)[last][0]==b[0]&&
       (*dihs)[last][1]==b[1]&&
       (*dihs)[last][2]==b[2]){
      b.setType((*dihs)[last].type());
      dihs->pop_back();
    }
    
    if(b[0]>=na||b[1]>=na||b[2]>=na||b[3]>=na)
      if(b[0]>=0&&b[1]>=0&&b[2]>=0&&b[3]>=0)
        dihs->push_back(b);
    
  }  
}

int addBegin(vector<AtomTopology> *atoms, 
	     BbEnd bb)
{
  int na=atoms->size();

  if(na==0){
    //we just add all the atoms
    for(int i=0; i<bb.numAtoms(); i++)
      atoms->push_back(bb.atom(i));
  }
  else{
    //we have to adapt the exclusions
    for(int i=0; i<bb.numAtoms(); i++){
      atoms->push_back(bb.atom(i));
    
      Exclusion e;
      for(int j=0; j<bb.atom(i).exclusion().size(); j++)
        e.insert(bb.atom(i).exclusion().atom(j)+na);
      (*atoms)[na+i].setExclusion(e);
    }
  }
  return bb.rep();
}

void addEnd(vector<AtomTopology> *atoms, 
	    BbEnd bb)
{
  int strt=atoms->size()+bb.rep();
  
   //we completely replace the last rep atoms
  for(int i=0;i<-bb.rep();i++)
    atoms->pop_back();
  
  //and we add our new atoms
  for(int i=0; i<bb.numAtoms(); i++){
    Exclusion e;
    for(int j=0; j<bb.atom(i).exclusion().size(); j++)
      e.insert(bb.atom(i).exclusion().atom(j)+strt);
    
    atoms->push_back(bb.atom(i));
    (*atoms)[strt+i].setExclusion(e);
  }
  
}

void addCovEnd(vector<Bond> *bonds, 
               vector<Angle> *angles, 
	       vector<Improper> *imps, 
	       vector<Dihedral> *dihs, BbEnd bb, int offset)
{
  int found=0;
  BeBondIt bi(bb);
  for(;bi;++bi){
    Bond b(bi()[0]+offset,bi()[1]+offset);
    b.setType(bi().type());
    if(bb.rep()<0){

      //search if this bond is present alread
      found=0;
      for(unsigned int i=0; i<bonds->size(); i++)
	if((*bonds)[i][0]==b[0]&&(*bonds)[i][1]==b[1]){
	  (*bonds)[i].setType(bi().type());
	  found=1;
	}
      if(!found) bonds->push_back(b);
    }
    else bonds->push_back(b);
  }

  //now, the angles
  BeAngleIt ai(bb);
  for(;ai;++ai){
    Angle b(ai()[0]+offset,ai()[1]+offset, ai()[2]+offset);
    b.setType(ai().type());

    //search if this angle is present alread
    found=0;
    for(unsigned int i=0; i<angles->size(); i++){
      if(b[0]<=offset){
	  
        if((*angles)[i][0]<=offset&&
	   (*angles)[i][1]==b[1]&&
	   (*angles)[i][2]==b[2]){
	  (*angles)[i].setType(b.type());
	  found=1;
	}
      }
      else{
	if((*angles)[i][0]==b[0]&&
	   (*angles)[i][1]==b[1]&&
	   (*angles)[i][2]==b[2]){
	  (*angles)[i].setType(b.type());
	  found=1;
	}
      }
    }
    if(!found) angles->push_back(b);
  }

  //impropers
  BeImpIt ii(bb);
  for(;ii;++ii){
    Improper b(ii()[0]+offset,ii()[1]+offset, ii()[2]+offset, ii()[3]+offset);
    b.setType(ii().type());

    //search if this improper is present alread
    found=0;
    int low=offset+1;
    for(int i=0;i<4;i++) if(b[i]<low) low=b[i];
    
    for(unsigned int i=0; i<imps->size(); i++){
      if(low<=offset){
        if((*imps)[i][0]<=offset&&
	   (*imps)[i][1]==b[1]&&
	   (*imps)[i][2]==b[2]&&
	   (*imps)[i][2]==b[3]){
	  (*imps)[i].setType(b.type());
	  found=1;
	}
      }
      else{
        if((*imps)[i][0]==b[0]&&
	   (*imps)[i][1]==b[1]&&
	   (*imps)[i][2]==b[2]&&
	   (*imps)[i][2]==b[3]){
	  (*imps)[i].setType(b.type());
	  found=1;
	}	
      }
    }
    if(!found) imps->push_back(b);
  } 

  //Dihedrals
  BeDihIt di(bb);
  for(;di;++di){
    int corr=offset;
    
    // search for the bonded atom if we specified negative numbers
    if(di()[0]==-1){
      for(unsigned int h=0; h<bonds->size(); h++)
	if((*bonds)[h][1]==di()[1]+offset) corr=(*bonds)[h][0];
      corr+=1;
    }

    Dihedral b(di()[0]+corr,di()[1]+offset, di()[2]+offset, di()[3]+offset);
    b.setType(di().type());

    //search if this dihedral is present alread
    found=0;
    for(unsigned int i=0; i<dihs->size(); i++){
     if(b[0]<=offset){
	if((*dihs)[i][0]<=offset&&
	   (*dihs)[i][1]==b[1]&&
	   (*dihs)[i][2]==b[2]&&
	   (*dihs)[i][3]==b[3]){
	  (*dihs)[i].setType(b.type());
	  found=1;
	}
     }
     else{
       	if((*dihs)[i][0]==b[0]&&
	   (*dihs)[i][1]==b[1]&&
	   (*dihs)[i][2]==b[2]&&
	   (*dihs)[i][2]==b[3]){
	  (*dihs)[i].setType(b.type());
	  found=1;
	}
      }
    }
    if(!found) dihs->push_back(b);
  } 
}
void removeAtoms(vector <AtomTopology> *atoms,
		 vector <Bond> *bonds,
		 vector <Angle> *angles,
		 vector <Improper> *imps,
		 vector <Dihedral> *dihs)
{
  set<int> rem;
  vector<int> ren;
  int corr=0;
  for(unsigned int i=0; i< atoms->size();i++){
    
    if ( (*atoms)[i].iac() < 0 ) {
      rem.insert(i);
      corr++;
      ren.push_back(-1);
    }
    else{
      ren.push_back(i-corr);
    }
  }
  if ( rem.size() == 0 ) return;

  // add four more to ren, in order to have a buffer
  for(int i=0; i<6; i++)
    ren.push_back(atoms->size()+i-corr);
  
  // process the atoms
  reduceAtoms(atoms, rem, ren);
  reduceBonds(bonds, rem, ren);
  reduceAngles(angles, rem, ren);
  reduceImps(imps, rem, ren);
  reduceDihs(dihs, rem, ren);
}

void reduceAtoms(vector <AtomTopology> *atoms, set<int> rem, vector<int> ren)
{
  int count=0;
  for(vector<AtomTopology>::iterator iter=atoms->begin(); 
      iter!=atoms->end();){
    if (rem.count(count)) atoms->erase(iter);
    else{
      Exclusion e;
      for(int j=0; j < iter->exclusion().size(); j++){
	if(!rem.count(iter->exclusion().atom(j)))
	  e.insert(ren[iter->exclusion().atom(j)]);
      }
      iter->setExclusion(e);
      ++iter;
    }
    count++;
  }
}

void reduceBonds(vector<Bond> *bonds, set<int> rem, vector<int> ren)
{
  int type;
  for(vector<Bond>::iterator iter=bonds->begin(); iter!=bonds->end();){
    if(rem.count((*iter)[0])||rem.count((*iter)[1])) bonds->erase(iter);
    else{
      type=iter->type();
      (*iter)=Bond(ren[(*iter)[0]], ren[(*iter)[1]]);
      iter->setType(type);
      ++iter;
    }
  }
}

void reduceAngles(vector<Angle> *angles, set<int> rem, vector<int> ren)
{
  int type;
  for(vector<Angle>::iterator iter=angles->begin(); iter!=angles->end();){
    if(rem.count((*iter)[0])||rem.count((*iter)[1])||rem.count((*iter)[2])) 
      angles->erase(iter);
    else{
      type=iter->type();
      (*iter)=Angle(ren[(*iter)[0]], ren[(*iter)[1]], ren[(*iter)[2]]);
      iter->setType(type);
      ++iter;
    }
  }
}

void reduceImps(vector<Improper> *imps, set<int> rem, vector<int> ren)
{
  int type;
  for(vector<Improper>::iterator iter=imps->begin(); iter!=imps->end();){
    if(rem.count((*iter)[0])||rem.count((*iter)[1])||
       rem.count((*iter)[2])||rem.count((*iter)[3])) 
      imps->erase(iter);
    else{
      type=iter->type();
      (*iter)=Improper(ren[(*iter)[0]], ren[(*iter)[1]], 
		       ren[(*iter)[2]], ren[(*iter)[3]]);
      iter->setType(type);
      ++iter;
    }
  }
}

void reduceDihs(vector<Dihedral> *dihs, set<int> rem, vector<int> ren)
{
  int type;
  for(vector<Dihedral>::iterator iter=dihs->begin(); iter!=dihs->end();){
    if(rem.count((*iter)[0])||rem.count((*iter)[1])||
       rem.count((*iter)[2])||rem.count((*iter)[3])) 
      dihs->erase(iter);
    else{
      type=iter->type();
      (*iter)=Dihedral(ren[(*iter)[0]], ren[(*iter)[1]], 
		       ren[(*iter)[2]], ren[(*iter)[3]]);
      iter->setType(type);
      ++iter;
    }
  }
}

void get14s(vector<AtomTopology> *atoms, vector<Bond> *bonds)
{
  int na=atoms->size();
  
  for(int i=0; i<na; i++){
    set<int> first, second, third;
    for(unsigned int j=0; j<bonds->size(); j++){
      if(i==(*bonds)[j][0]) first.insert((*bonds)[j][1]);
      if(i==(*bonds)[j][1]) first.insert((*bonds)[j][0]);
    }
    for(set<int>::const_iterator iter=first.begin(), to=first.end();
	iter!=to; ++iter){
      for(unsigned int j=0; j<bonds->size(); j++){
	if(*iter==(*bonds)[j][0]) second.insert((*bonds)[j][1]);
	if(*iter==(*bonds)[j][1]) second.insert((*bonds)[j][0]);
      }
    }
    for(set<int>::const_iterator iter=second.begin(), to=second.end();
	iter!=to; ++iter){
      for(unsigned int j=0; j<bonds->size(); j++){
	if(*iter==(*bonds)[j][0]) third.insert((*bonds)[j][1]);
	if(*iter==(*bonds)[j][1]) third.insert((*bonds)[j][0]);
      }
    }
    Exclusion e;
    for(set<int>::const_iterator iter=third.begin(), to=third.end();
	iter!=to; ++iter){
      if(i<*iter&&
	 !first.count(*iter)&&
	 !second.count(*iter)){
        int excl=0;
	for(int k=0; k<(*atoms)[i].exclusion().size();k++)
	  if(*iter==(*atoms)[i].exclusion().atom(k)) excl=1;
	if(!excl) e.insert(*iter);
      }
    }
    (*atoms)[i].setExclusion14(e);
  }
}
System parseTopology(vector<AtomTopology> *atoms, 
		     vector<Bond> *bonds, 
		     vector<Angle> *angles, 
		     vector<Improper> *imps, 
		     vector<Dihedral> *dihs, 
		     vector<string> *resNames, 
		     map<int,int> *resMap)
{
  // largely copied from InTopology
  int last1=0, last=0, lastres=0;
  MoleculeTopology *mt;
  System sys;
    
  while(atoms->size()){
    mt=new MoleculeTopology();

    // Detect the last atom of the first molecule & add bonds:
    for(vector<Bond>::iterator iter=bonds->begin();
          (iter!=bonds->end())&& (*iter)[0] <=last ; ){
      Bond bond=*iter;
      if(bond[1]>last)last=bond[1];
      bond[0]-=last1;
      bond[1]-=last1;
      mt->addBond(bond);
      bonds->erase(iter);
    }
    last++;
    
    // add Atoms
    for(int i=0;i<last-last1; ++i){
	
      // adapt exclusions:
      Exclusion *e;
      e=new Exclusion();
      for (int l=0;l<(*atoms)[0].exclusion().size();++l)
        e->insert((*atoms)[0].exclusion().atom(l)-last1);
      (*atoms)[0].setExclusion(*e);
	
      delete e;
      e=new Exclusion();
      for (int l=0;l<(*atoms)[0].exclusion14().size();++l)
        e->insert((*atoms)[0].exclusion14().atom(l)-last1);
      (*atoms)[0].setExclusion14(*e);
      delete e;
      
      // now add atom, set residue number and name
      mt->addAtom((*atoms)[0]);
      atoms->erase(atoms->begin());

      int resn=(*resMap)[i+last1]-lastres;
      mt->setResNum(i,resn);
      mt->setResName(resn,(*resNames)[resn+lastres]);
    }
    lastres+=mt->numRes();
    
    // add Angles
    for(vector<Angle>::iterator iter=angles->begin();
        iter != angles->end() && (*iter)[0]<last;){
      Angle angle(*iter);
      angle[0]-=last1;
      angle[1]-=last1;
      angle[2]-=last1;
      mt->addAngle(angle);
      angles->erase(iter);
    }
      
    // add Dihedrals
    for(vector<Dihedral>::iterator iter=dihs->begin();
        iter != dihs->end() && (*iter)[0]<last;){
      Dihedral dihedral(*iter);
      dihedral[0]-=last1;
      dihedral[1]-=last1;
      dihedral[2]-=last1;
      dihedral[3]-=last1;
      mt->addDihedral(dihedral);
      dihs->erase(iter);
    }
	
    // add Impropers
    for(vector<Improper>::iterator iter=imps->begin();
        iter != imps->end() && (*iter)[0]<last;){
      Improper improper(*iter);
      improper[0]-=last1;
      improper[1]-=last1;
      improper[2]-=last1;
      improper[3]-=last1;
      mt->addImproper(improper);
      imps->erase(iter);
    }

    // add the molecule to the system.      
    sys.addMolecule(Molecule(*mt));
    delete mt;
    last1=last;
  }
  return sys;
}

void setCysteines(vector<AtomTopology> *atoms,
		  vector<Bond> *bonds,
		  vector<Angle> *angles,
		  vector<Improper> *imps,
		  vector<Dihedral> *dihs,
		  int a, int b)
{
  // brace yourselves, this will be ugly!

  // exclusions
  for(int i=a+1; i<a+3; i++){
    Exclusion e;
    for(int j=0; j<(*atoms)[i].exclusion().size();j++){
      if((*atoms)[i].exclusion().atom(j)<a)
	e.insert(a+b-6-(*atoms)[i].exclusion().atom(j));
      else
	e.insert((*atoms)[i].exclusion().atom(j));
    }
    (*atoms)[i].setExclusion(e);
  }

  // bond
  int added=0, bt=0;
  
  for(vector<Bond>::iterator iter=bonds->begin(); iter!=bonds->end(); ++iter){

    if((*iter)[0]==a-8&&(*iter)[1]==a+2){
      bt=iter->type();
      bonds->erase(iter);
    }
    // wait with adding the corrected bond, to prevent later errors with
    // parsing    
    if(!added&&(*iter)[0]==a+1&&(*iter)[1]==a+2){
      added=1;
      Bond bb(a+2, b+2);
      bb.setType(bt);
      bonds->insert(iter, bb);
    }
  }
  
  //two angles
  for(vector<Angle>::iterator iter=angles->begin(); 
      iter!=angles->end(); ++iter){
    if((*iter)[0]==a-8&&(*iter)[1]==a+2&&(*iter)[2]==a+1){
      Angle bb(a+1, a+2, b+2);
      bb.setType(iter->type());
      (*iter)=bb;
    }
  }
  for(vector<Angle>::iterator iter=angles->begin(); 
      iter!=angles->end(); ++iter){
    if((*iter)[0]==a-7&&(*iter)[1]==a-8&&(*iter)[2]==a+2){
      Angle bb(a+2, b+2, b+1);
      bb.setType(iter->type());
      (*iter)=bb;
    }
  }

  //three dihedrals
  for(vector<Dihedral>::iterator iter=dihs->begin(); 
      iter!=dihs->end(); ++iter){
    if((*iter)[0]==a&&(*iter)[1]==a+1&&(*iter)[2]==a+2&&(*iter)[3]==a-8){
      Dihedral bb(a, a+1, a+2, b+2);
      bb.setType(iter->type());
      (*iter)=bb;
    }
  }  
  for(vector<Dihedral>::iterator iter=dihs->begin(); 
      iter!=dihs->end(); ++iter){
    if((*iter)[0]==a-7&&(*iter)[1]==a-8&&(*iter)[2]==a+2&&(*iter)[3]==a+1){
      Dihedral bb(a+1, a+2, b+2, b+1);
      bb.setType(iter->type());
      (*iter)=bb;
    }
  }  
  for(vector<Dihedral>::iterator iter=dihs->begin();
      iter!=dihs->end(); ++iter){
    if((*iter)[0]==a+2&&(*iter)[1]==a-8&&(*iter)[2]==a-7&&(*iter)[3]==a-6){
      Dihedral bb(a+2, b+2, b+1, b);
      bb.setType(iter->type());
      (*iter)=bb;
    }
  }
}



