// gcore_MoleculeTopology.h

#ifndef INCLUDED_GCORE_MOLECULETOPOLOGY
#define INCLUDED_GCORE_MOLECULETOPOLOGY

#ifndef INCLUDED_STRING
#include <string>
#define INCLUDED_STRING
#endif

namespace gcore{

  class MoleculeTopology_i;
  class GromosForceField;
  class AtomTopology;
  class Bond;
  class Angle;
  class Dihedral;
  class Improper;
  class BondIterator;
  class AngleIterator;
  class ImproperIterator;
  class DihedralIterator;
  /**
   * Class MoleculeTopology
   * Purpose: Contains all topological information for a Molecule
   *
   * Description:
   * The MoleculeTopology contains all topological information for a 
   * Molecule. For the Atoms there is a direct accessor to the 
   * AtomTopologies. Bonds, Angles etc. you access via the BondIterator, 
   * AngleIterator, ImproperIterator and DihedralIterator.
   *
   * @class MoleculeTopology
   * @author R. Buergi
   * @ingroup gcore
   * @sa gcore::Molecule
   * @sa gcore::AtomTopology
   * @sa gcore::Bond
   * @sa gcore::Angle
   * @sa gcore::Improper
   * @sa gcore::Dihedral
   */
  class MoleculeTopology{
    MoleculeTopology_i *d_this;
    // This class contains all topological information
    /**
     * Bond Iterator for the a MoleculeTopology
     *
     * The MoleculeTopology Bond iterator is used to loop over the Bonds 
     * in a MoleculeTopology. 
     * It is constructed with the MoleculeTopology as an argument. Use the 
     * ++ operator to move to the next Bond. The () operator returns the 
     * current Bond. 
     * This can also be used as a boolean: the bool() returns 1 as long as 
     * the iterator is not at the end of the bond list.
     * @author R. Buergi
     */
    friend class BondIterator;
    /**
     * Angle Iterator for the a MoleculeTopology
     *
     * The MoleculeTopology Angle iterator is used to loop over the Angles 
     * in a MoleculeTopology. 
     * It is constructed with the MoleculeTopology as an argument. Use the 
     * ++ operator to move to the next Angle. The () operator returns the 
     * current Angle. 
     * This can also be used as a boolean: the bool() returns 1 as long as 
     * the iterator is not at the end of the angle list.
     * @author R. Buergi
     */
    friend class AngleIterator;
    /**
     * Improper Iterator for the a MoleculeTopology
     *
     * The MoleculeTopology Improper iterator is used to loop over the 
     * Impropers in a MoleculeTopology. 
     * It is constructed with the MoleculeTopology as an argument. Use the 
     * ++ operator to move to the next Improper. The () operator returns the 
     * current Improper. 
     * This can also be used as a boolean: the bool() returns 1 as long as 
     * the iterator is not at the end of the improper list.
     * @author R. Buergi
     */
    friend class ImproperIterator;
    /**
     * Dihedral Iterator for the a MoleculeTopology
     *
     * The MoleculeTopology Dihedral iterator is used to loop over the 
     * Dihedrals in a MoleculeTopology. 
     * It is constructed with the MoleculeTopology as an argument. Use the 
     * ++ operator to move to the next Dihedral. The () operator returns the 
     * current Dihedral. 
     * This can also be used as a boolean: the bool() returns 1 as long as 
     * the iterator is not at the end of the Dihedral list.
     * @author R. Buergi
     */
    friend class DihedralIterator;
    


  public:
    /**
     * MoleculeTopology constructor
     */
    MoleculeTopology();
    /**
     * MoleculeTopology copy constructor
     * @param & MoleculeTopology to be copied
     */
    MoleculeTopology(const MoleculeTopology &);
    /**
     * MoleculeTopology deconstructor
     */
    ~MoleculeTopology();
    
    /**
     * MoleculeTopology = operator
     */
    MoleculeTopology &operator=(const MoleculeTopology &); 

    // Methods
    /**
     * Method to add an atom to the MoleculeTopology
     * @param a An AtomTopology that is to be added; should be complete
     *          already
     */
    void addAtom(const AtomTopology &a);
    /**
     * Method to add a Bond to the MoleculeTopology
     * @param b The Bond that is to be added; should be complete already
     */
    void addBond(const Bond &b);
    /**
     * Method to add an Angle to the MoleculeTopology
     * @param b The Angle that is to be added; should be complete already
     */
    void addAngle(const Angle &b);
    /**
     * Method to add a Dihedral to the MoleculeTopology
     * @param b The Dihedral that is to be added; should be complete already
     */
    void addDihedral(const Dihedral &b);
    /**
     * Method to add an Improper to the MoleculeTopology
     * @param b The Improper that is to be added; should be complete already
     */
    void addImproper(const Improper &b);
    /**
     * Method to set the residue name
     * 
     * Within the MoleculeTopology a list of residues is kept so that you
     * can assign specific atoms to specific residues
     * @param res The number of the residue that gets a name
     * @param s   The name of this residue
     */
    void setResName(int res, const std::string &s);
    /**
     * Method to assign an atom to a residue
     *
     * Within the MoleculeTopology a list of residues is known, this method
     * allows you to assign an atom to a specific residue
     * @param atom The atom number of the atom to be assigned
     * @param res  The residue number to which the atom is assigned
     */
    void setResNum(int atom, int res);
    /**
     * Method to determine which atoms are hydrogens based on the mass
     */
    void setHmass(double mass);
    /**
     * Method to determine which atoms are hydrogens based on the iac
     */
    void setHiac(int iac);
    /**
     * Method to clear all isH flags of the atoms
     */
    void clearH();
    
    // Accessors
    /**
     * Accessor, returns the number of atoms in the MoleculeTopology
     */
    int numAtoms()const;
    /**
     * Accessor, return the AtomTopology of the i-th atom in the 
     * MoleculeTopology
     */
    AtomTopology & atom(int i);
    /**
     * Accessor, return the AtomTopology of the i-th atom in the 
     * MoleculeTopology as a const
     */
    const AtomTopology& atom(int i) const; 
    /**
     * Accessor, returns the number of residues in the molecule
     */
    int numRes()const;
    /**
     * Accessor, returns the residue number to which the atom belongs
     * @param atom the atom number of an atom in the topology
     * @return the number of the residue to which this atom belongs
     */
    int resNum(int atom) const;
    /**
     * Accessor, returns the name of a residue number
     * @param i The number of a residue in the MoleculeTopology
     * @return A string with the name of the residue
     */
    const std::string &resName(int i)const;


  }; /* class MoleculeTopology */


  class BondIterator_i;
  class AngleIterator_i;
  class ImproperIterator_i;
  class DihedralIterator_i;

  class BondIterator{
    BondIterator_i *d_this;
    // not implemented
    BondIterator();
    BondIterator(const BondIterator&);
    BondIterator &operator=(const BondIterator &);
  public:
    BondIterator(const MoleculeTopology &mt);
    ~BondIterator();
    void operator++();
    const Bond &operator()()const;
    operator bool()const;
  };

  class AngleIterator{
    AngleIterator_i *d_this;
    // not implemented
    AngleIterator();
    AngleIterator(const AngleIterator&);
    AngleIterator &operator=(const AngleIterator &);
  public:
    AngleIterator(const MoleculeTopology &mt);
    ~AngleIterator();
    void operator++();
    const Angle &operator()()const;
    operator bool()const;
  };

  class ImproperIterator{
    ImproperIterator_i *d_this;
    // not implemented
    ImproperIterator();
    ImproperIterator(const ImproperIterator&);
    ImproperIterator &operator=(const ImproperIterator &);
  public:
    ImproperIterator(const MoleculeTopology &mt);
    ~ImproperIterator();
    void operator++();
    const Improper &operator()()const;
    operator bool()const;
  };

  class DihedralIterator{
    DihedralIterator_i *d_this;
    // not implemented
    DihedralIterator();
    DihedralIterator(const DihedralIterator&);
    DihedralIterator &operator=(const DihedralIterator &);
  public:
    DihedralIterator(const MoleculeTopology &mt);
    ~DihedralIterator();
    void operator++();
    const Dihedral &operator()()const;
    operator bool()const;
  };

} /* Namespace */ 
#endif



