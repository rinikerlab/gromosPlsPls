// gcore_BuildingBlock.h

#ifndef INCLUDED_GCORE_BUILDINGBLOCK
#define INCLUDED_GCORE_BUILDINGBLOCK

#ifndef INCLUDED_VECTOR
#include <vector>
#define INCLUDED_VECTOR
#endif

namespace gcore{

  class BbSolute;
  class BbEnd;
  class SolventTopology;
  /**
   * Class BuildingBlock
   * Purpose: contains all different kinds of building blocks
   *
   * Description:
   * The BuildingBlock class contains all building blocks and additional
   * information from the gromos96 mtb-file. Three different kinds of
   * building blocks are known: MTBUILDBLSOLUTE (class BbSolute), 
   * MTBUILDBLSOLVENT (class SolventTopology) and MTBUILDBLEND (class 
   * BbEnd)
   *
   * @class BuildingBlock
   * @author C. Oostenbrink
   * @version $Date: Wed Jul 10 14:00:00 MEST 2002
   * @sa gcore::BbSolute
   * @sa gcore::SolventTopology
   * @sa gcore::BBEnd
   */
  class BuildingBlock{
    std::vector<BbSolute*> d_bb;
    std::vector<BbEnd*> d_be;
    std::vector<SolventTopology*> d_bs;
    double d_fpepsi;
    double d_hbar;
    int d_linkExclusions;
    

  public:
    //Constructors
    /**
     * BuildingBlock constructor
     */
    BuildingBlock();
    /**
     * BuildingBlock copy constructor
     * @param bld BuildingBlock to be copied
     */
    BuildingBlock(const BuildingBlock &bld);
    /**
     * BuildingBlock deconstructor
     */
    ~BuildingBlock();

    // Methods
    /**
     * Member function operator = copies one set of building blocks
     * into the other
     */
    BuildingBlock &operator=(const BuildingBlock &bld);
    /**
     * Method to add a solute building block to the building blocks
     * @param mol a BbSolute (corresponds to a MTBUILDBLSOLUTE block)
     */
    void addBbSolute(const BbSolute &mol);
    /**
     * Method to add a solvent building block to the building blocks
     * @param sol a SolventTopology (corresponds to a MTBUILDBLSOLVENT 
     *        block
     */
    void addBbSolvent(const SolventTopology &sol);
    /**
     * Method to add an end group building block to the building blocks
     * @param mol a BbEnd (corresponds to a MTBUILDBLEND block)
     */
    void addBbEnd(const BbEnd &mol);
    /**
     * Set the value of Fpepsi. It would probably make more sense to store
     * Fpepsi in the GromosForceField, but this is the gromos96 structure
     */
    void setFpepsi(const double a){d_fpepsi=a;};
    /**
     * Set the value of Hbar. It would probably make more sense to store 
     * Hbar in the GromosForceField, but this is the gromos96 structure
     */
    void setHbar(const double a){d_hbar=a;};
    /**
     * Set the number of exclusions when linking (= number of trailing
     * atoms)
     */
    void setLinkExclusions(const int i){d_linkExclusions=i;};
    
    
    // Accessors
    /**
     * Accessor, returns the i-th BbSolute as a const
     */
    const BbSolute &bb(int i)const;
    /**
     * Accessor, returns the i-th BbSolute
     */
    BbSolute &bb(int i);
    /**
     * Accessor, returns the i-th BbEnd as a const
     */
    const BbEnd &be(int i)const;
    /**
     * Accessor, returns the i-th BbEnd
     */
    BbEnd &be(int i);
    /**
     * Accessor, returns the i-th SolventTopology as a const
     */
    const SolventTopology &bs(int i)const;
    /**
     * Accessor, returns the i-th SolventTopology
     */
    SolventTopology &bs(int i);

    /**
     * Accessor, returns the number of BbSolutes in the BuildingBlock
     */
    int numBbSolutes()const;
    /**
     * Accessor, returns the number of BbSolvents (SolventTopologies) in 
     * the BuildingBlock
     */
    int numBbSolvents()const;
    /**
     * Accessor, returns the number of BbEnds in the BuildingBlock
     */
    int numBbEnds()const;
    /**
     * Accessor, returns the value of Fpepsi
     */
    double Fpepsi()const;
    /**
     * Accessor, returns the value of Hbar
     */
    double Hbar()const;
    /**
     * Accessor, returns the number of exclusions for linking
     */
    int LinkExclusions()const;
    /**
     * Method, returns an index for the first building block that is 
     * has the name s. This method searches through both the solute
     * and the end-group building blocks
     * @param s String with the building block name to search for
     * @return integer i with value<br>
     *         0  if s is not found<br>
     *         >0 s is found as the (i-1)-th solute building block.
     *            <i>Don't forget to substract the 1!</i><br>
     *         <0 s is found as the (abs(i)-1)-th end-group building 
     *            block. <i>Don't forget to convert i to the index!</i>
     */ 
    int findBb(std::string s);
    /** 
     * Method, returns an index for the first solvent building block that
     * has the name s.
     * @param s String with the solvent building block name to search for
     * @return integer i with value<br>
     *         0  if s is not found<br>
     *         >0 s is found as the (i-1)-th solvent building block.
     *            <i>Don't forget to substract the 1!</i>
     */
    int findBs(std::string s);
    
    
};

  inline const BbSolute &BuildingBlock::bb(int i)const{
    assert (i < this->numBbSolutes());
    return *d_bb[i];
  }

  inline BbSolute &BuildingBlock::bb(int i){
    assert (i < this->numBbSolutes());
    return *d_bb[i];
  }

  inline const BbEnd &BuildingBlock::be(int i)const{
      assert (i < this->numBbEnds());
      return *d_be[i];
  }

  inline BbEnd &BuildingBlock::be(int i){
      assert (i < this->numBbEnds());
      return *d_be[i];
  }

  inline const SolventTopology &BuildingBlock::bs(int i)const{
      assert (i< this->numBbSolvents());
      return *d_bs[i];
  }

  inline SolventTopology &BuildingBlock::bs(int i){
      assert (i < this->numBbSolvents());
      return *d_bs[i];
  }

  inline int BuildingBlock::numBbSolutes()const{
    return d_bb.size();
  }

  inline int BuildingBlock::numBbEnds()const{
      return d_be.size();
  }

  inline int BuildingBlock::numBbSolvents()const{
    return d_bs.size();
  }

  inline double BuildingBlock::Fpepsi()const{
    return d_fpepsi;
  }
  
  inline double BuildingBlock::Hbar()const{
    return d_hbar;
  }
  
  inline int BuildingBlock::LinkExclusions()const{
    return d_linkExclusions;
  }
  
  
}
#endif

