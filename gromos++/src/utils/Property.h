/**
 * @file Property.h
 * Properties and PropertySpecifiers
 */

/* 	$Id$	 */

#ifndef MAXFLOAT
#define	MAXFLOAT	((float)3.40282346638528860e+38)
#endif

#ifndef INCLUDED_UTILS_PROPERTY
#define INCLUDED_UTILS_PROPERTY

#ifndef INCLUDED_VECTOR
#include <vector>
#define INCLUDED_VECTOR
#endif

#ifndef INCLUDED_STRING
#include <string>
#define INCLUDED_STRING
#endif

#ifndef INCLUDED_GROMOS_EXCEPTION
#include "../gromos/Exception.h"
#endif

#ifndef INCLUDED_VECTOR_SPECIFIER
#include "VectorSpecifier.h"
#endif

namespace gcore
{
  class System;
  class Molecule;
  class MoleculeTopology;
}

namespace bound
{
  class Boundary;
}

namespace utils
{
  class Property;
  std::ostream &operator<<(std::ostream &os, Property &p);

  /**
   * @class Property
   * @version Jul 31 15:00 2002
   * @author M. Christen
   * @ingroup utils
   * @sa utils::PropertyContainer
   * @sa utils::DistanceProperty
   * @sa utils::AngleProperty
   * @sa utils::TorsionProperty
   * Class Property 
   * Purpose: General base class for properties to be calculated
   * during an analysis run.
   *
   * Description:
   * This class defines (and implements) the general methods that a
   * property calculated during an analysis run should posses.
   * Specialized derived classes for (ie) bond lengths, angles or
   * dihedral angles exist.
   * Also two classes for calculating order paramters are there.
   * Ask Indira what they do...
   *
   * @section PropertySpecifier Property Specifier
   * A property specifier is of the general form:<br>
   * <br>
   * <span style="color:darkred;font-size:larger"><b>
   * @verbatim type%<AtomSpecifier>%zero_value%lower_bound%upper_bound @endverbatim
   * </b></span>
   * <br>
   * type can be:
   * - <b>d</b> @ref DistanceProperty "Distance"
   * - <b>a</b> @ref AngleProperty "Angle"
   * - <b>t</b> @ref TorsionProperty "Torsion"
   * - <b>o</b> @ref OrderProperty "Order"
   * - <b>vo</b> @ref VectorOrderProperty "Vector order"
   * - <b>op</b> @ref OrderParamProperty "Order parameter"
   * - <b>vop</b> @ref VectorOrderParamProperty "Vector order parameter"
   * - <b>pr</b> @ref PseudoRotationProperty "Pseudo rotation"
   * - <b>pa</b> @ref PuckerAmplitudeProperty "Pucker amplitude"
   *
   * The @ref AtomSpec "atom specifiers" should list the number of necessary atoms.
   * A <b>zero value</b> can be specified if deviation from this value is 
   * interesting.
   * If <b>lower</b> and <b>upper boundaries</b> are given, for any values outside the
   * boundaries, a message is printed to the output file
   * (for programs that activate this feature).
   * 
   * <b>See also</b> @ref AtomSpecifier "Atom Specifier"
   */
  class Property
  {
  public:
    /**
     * Maximum number of arguments.
     */
    static const int MAXARGUMENTS = 10;

    /**
     * Standard constructor. Not used for predefined properties.
     * (Because they need all a reference to the system to be able
     * to calculate their value)
     */
    // Property(); // for user defined properties that do not need a system
    /**
     * Standard constructor.
     * Takes a reference to the system, so that the properties can
     * calculate their value (ie by looking up the atom positions).
     */
    Property(gcore::System &sys, bound::Boundary * pbc);
    /**
     * Destructor.
     */
    virtual ~Property();

    // accessor
    std::string type() {return d_type;}

    /**
     * Return the value of the property.
     * In retrospect, i should have written the whole thing as a
     * template, so that anything could be returned here.
     * Maybe sometime somebody wants to change that.
     */
    double getValue();
    /**
     * Returns the ideal value of the property (if one has been specified).
     */
    double getZValue();
    /**
     * Returns the minimum value of an allowed range (if one has been 
     * specified).
     */
    double getMinValue();
    /**
     * Returns the maximum value of an allowed range (if one has been 
     * specified).
     */
    double getMaxValue();
    /**
     * As most of the properties i can think of have something to do with
     * atoms and molecules, i define these vectors in the base class.
     * This is also done in order to be able to write one general
     * arguments parsing function in the base class.
     */
    // std::vector<int> atoms();
    /**
     * Vector of the molecule mol[i] corresponding to atom[i].
     * @sa utils::Property::atoms
     */
    // std::vector<int> mols();

    AtomSpecifier & atoms();

    // methods
    
    /**
     * Calculates the value of the property.
     * Override in derived classes.
     */
    virtual double calc();
    /**
     * After a call to utils::Property::calc() checkBounds() can check,
     * whether the value of the property lies within the specified range.
     * (This only works if calc() calculates one single value and stores
     * it into d_value)
     */
    virtual std::string checkBounds();
    /**
     * Write a title string specifying the property.
     */
    virtual std::string toTitle();
    /**
     * Returns the value in string representation.
     */
    virtual std::string toString();
    /**
     * Returns the average value over all calls to calc.
     */
    virtual std::string average();

    /**
     * Returns the type of the interaction from the
     * topology.
     */
    virtual int getTopologyType(gcore::System const &sys);

    /**
     * @struct Exception
     * Property exception
     */
    struct Exception: public gromos::Exception
    {
      /**
       * Constructor.
       */
      Exception(const std::string &what): 
	gromos::Exception("Property", what) {}
    };
    
  protected:
    /**
     * Parse the command line property specification.
     * This is the standard implementation. It knows about
     * molecules, atoms, zero value and boundaries.
     */
    virtual void parse(int count, std::string arguments[]);
    /**
     * Helper method to parse the atom part of the property specifier.
     * The argument is in AtomSpecifier format.
     */
    void parseAtoms(std::string atoms);
    /**
     * Helper method to parse the atoms belonging to one molecule.
     */
    // void _parseAtomsHelper(std::string substring, int &mol);

    /**
     * find the corresponding forcefield type of the property.
     * needs to be overwritten for the specific properties.
     */
    virtual int findTopologyType(gcore::MoleculeTopology const &mol_topo);
    
  protected:
    // member variables
    /**
     * Number of required arguments. Used in the standard parse function.
     * Set in the constructor.
     */
    int REQUIREDARGUMENTS; 
    /**
     * The property type (in string representation).
     */
    std::string d_type;

    /**
     * The bounds for 'boundary violation check'.
     * @sa utils::Property::checkBounds
     */
    double d_ubound, d_lbound;
    /**
     * The zero/equilibrium/standard value of the property.
     */
    double d_zvalue;
    /**
     * Stores the calculated value. This is used for subsequent toString
     * calls.
     * If in a user defined property, d_value is not used, those functions
     * must be overridden.
     */
    double d_value;
    
    /**
     * The atoms belonging to this property.
     */
    // std::vector<int> d_atom;
    /**
     * The molecule, the atoms belong to.
     */
    // std::vector<int> d_mol;
    AtomSpecifier d_atom;
    
    /**
     * Reference of the system.
     */
    gcore::System *d_sys;
    /**
     * pointer to a boundary object
     */
    bound::Boundary *d_pbc;

  };

  
  /**
   * Class DistanceProperty
   * Purpose: Implements a distance property.
   *
   * Description:
   * This class implements a distance property. It is derived from the 
   * Property class.
   * <br>
   * <span style="color:darkred;font-size:larger"><b>
   * @verbatim d%<AtomSpecifier>%zero_value%lower_bound%upper_bound @endverbatim
   * </b></span>
   * <br>
   *
   * @class DistanceProperty
   * @version Wed Jul 31 2002
   * @author gromos++ development team
   * @sa utils::Property
   */

  class DistanceProperty : public Property
  {    
  public:
    /**
     * Constructor.
     */
    DistanceProperty(gcore::System &sys, bound::Boundary * pbc);
    /**
     * Destructor.
     */
    virtual ~DistanceProperty();
    /**
     * Parses the arguments. Is overridden to check the input, but basically
     * just calls Property::parse.
     * @sa utils::Property::parse
     */
    virtual void parse(int count, std::string arguments[]);
    /**
     * Calculates the distance.
     */
    virtual double calc();
    /**
     * Averages over the calculated values.
     * Not implemented ?! (in class Property?!)
     */
    virtual std::string average();
    /**
     * Get the average value of all calc() calls.
     */
    // virtual std::string toString();
    /**
     * Get a title string.
     */
    // virtual std::string toTitle();
    /**
     * @struct Exception
     * DistanceProperty exception.
     */
    struct Exception: public gromos::Exception
    {
      /**
       * Constructor.
       */
      Exception(const std::string &what): 
	gromos::Exception("DistanceProperty", what) {}
    };

  protected:

    /**
     * find the corresponding forcefield type of the property.
     * needs to be overwritten for the specific properties.
     */
    virtual int findTopologyType(gcore::MoleculeTopology const &mol_topo);

    /**
     * The average value.
     */
    double d_average;
    /**
     * The rmsd from the zero/initial/ideal value.
     */
    double d_zrmsd;
    /**
     * The number of times that calc() has been called.
     */
    int d_count;
    
  };

  /**
   * Class AngleProperty
   * Purpose: Implements an angle property.
   *
   * Description:
   * Implements an angle property. It is derived from the Property class.
   * <br>
   * <span style="color:darkred;font-size:larger"><b>
   * @verbatim a%<AtomSpecifier>%zero_value%lower_bound%upper_bound @endverbatim
   * </b></span>
   * <br>
   *
   * @class AngleProperty
   * @version Wed Jul 31 2002
   * @author gromos++ development team
   * @sa utils::Property
   */

  class AngleProperty : public Property
  {
  public:
    /**
     * Constructor.
     */
    AngleProperty(gcore::System &sys, bound::Boundary * pbc);
    /**
     * Destructor.
     */
    virtual ~AngleProperty();
    /**
     * Parse and check property specifier (given in arguments).
     * Calls Property::parse and checks the arguments.
     */
    virtual void parse(int count, std::string arguments[]);
    /**
     * Calculate the angle between the given atoms.
     */
    virtual double calc();
    /**
     * Calculate the average of all values calculated so far.
     */
    virtual std::string average();
    /**
     * Print the calculated value.
     */
    // virtual std::string toString();
    /**
     * Get a title string.
     */
    // virtual std::string toTitle();
    /**
     * @struct Exception
     * AngleProperty exception.
     */
    struct Exception: public gromos::Exception
    {
      /**
       * Constructor.
       */
      Exception(const std::string &what): 
	gromos::Exception("AngleProperty", what) {}
    };
      
  protected:
    /**
     * find the corresponding forcefield type of the property.
     * needs to be overwritten for the specific properties.
     */
    virtual int findTopologyType(gcore::MoleculeTopology const &mol_topo);

    /**
     * The average value.
     */
    double d_average;
    /**
     * The rmsd from the ideal value.
     */
    double d_zrmsd;    // <zero value> rmsd
    /**
     * Number of times calc() has been called.
     */
    int d_count;
      
  };
      
  /**
   * Class TorsionProperty
   * Purpose: Implements a torsion property.
   *
   * Description:
   * This class implements a torsion property. It is derived from the 
   * Property class.
   * <br>
   * <span style="color:darkred;font-size:larger"><b>
   * @verbatim t%<AtomSpecifier>%zero_value%lower_bound%upper_bound @endverbatim
   * </b></span>
   * <br>
   *
   * @class TorsionProperty
   * @version Wed Jul 31 2002
   * @author gromos++ development team
   * @sa utils::Property
   */

  class TorsionProperty : public Property
  {
  public:
    /**
     * Constructor.
     */
    TorsionProperty(gcore::System &sys, bound::Boundary * pbc);
    /**
     * Destructor.
     */
    virtual ~TorsionProperty();
    /**
     * Parse the arguments. Calls Property::parse.
     */
    virtual void parse(int count, std::string arguments[]);
    /**
     * Calculate the torsional angle.
     */
    virtual double calc();
    /**
     * Get the average of the calculated values.
     */
    virtual std::string average();
    /**
     * Get the value as string.
     */
    // virtual std::string toString();
    /**
     * Get a title string.
     */
    // virtual std::string toTitle();
    /**
     * @struct Exception
     * TorsionProperty exception.
     */
    struct Exception: public gromos::Exception
    {
      /**
       * Constructor.
       */
      Exception(const std::string &what): 
	gromos::Exception("TorsionProperty", what) {}
    };
      
  protected:
    /**
     * find the corresponding forcefield type of the property.
     * needs to be overwritten for the specific properties.
     */
    virtual int findTopologyType(gcore::MoleculeTopology const &mol_topo);

    /**
     * The average value.
     */
    double d_average;
    /**
     * The rmsd from the ideal value.
     */
    double d_zrmsd;
    /**
     * Number of times calc() has been called.
     */
    int   d_count;
  };    

  /**
   * Class OrderProperty
   * Purpose: Implements an order property.
   *
   * Description:
   * Implements an order property
   * (angle between axis and vector specified
   * by two atoms).
   * It is derived from the Property class.
   * <br>
   * <span style="color:darkred;font-size:larger"><b>
   * @verbatim o%<AtomSpecifier>%zero_value%lower_bound%upper_bound @endverbatim
   * </b></span>
   * <br>
   *
   * @class OrderProperty
   * @version Jan 16 2004
   * @author gromos++ development team
   * @sa utils::Property
   */

  class OrderProperty : public Property
  {
  public:
    /**
     * Constructor.
     */
    OrderProperty(gcore::System &sys, bound::Boundary * pbc);
    /**
     * Destructor.
     */
    virtual ~OrderProperty();
    /**
     * Parse and check property specifier (given in arguments).
     * Calls Property::parse and checks the arguments.
     */
    virtual void parse(int count, std::string arguments[]);
    /**
     * Calculate the angle between the given atoms.
     */
    virtual double calc();
    /**
     * Calculate the average of all values calculated so far.
     */
    virtual std::string average();
    /**
     * Print the calculated value.
     */
    // virtual std::string toString();
    /**
     * Get a title string.
     */
    virtual std::string toTitle();
    /**
     * @struct Exception
     * AngleProperty exception.
     */
    struct Exception: public gromos::Exception
    {
      /**
       * Constructor.
       */
      Exception(const std::string &what): 
	gromos::Exception("OrderProperty", what) {}
    };
      
  protected:
    /**
     * find the corresponding forcefield type of the property.
     * needs to be overwritten for the specific properties.
     */
    virtual int findTopologyType(gcore::MoleculeTopology const &mol_topo);

    /**
     * The average value.
     */
    double d_average;
    /**
     * The rmsd from the ideal value.
     */
    double d_zrmsd;    // <zero value> rmsd
    /**
     * Number of times calc() has been called.
     */
    int d_count;
    /**
     * axis with respect to which the angle is calculated.
     */
    gmath::Vec d_axis;
      
  };

  /**
   * Class VectorOrderProperty
   * Purpose: Implements a vector order property.
   *
   * Description:
   * Implements a vector order property
   * (angle between two specified vectors)
   * It is derived from the Property class.
   * <br>
   * <span style="color:darkred;font-size:larger"><b>
   * @verbatim vo%<VectorSpecifier>%<VectorSpecifier> @endverbatim
   * </b></span>
   * <br>
   *
   * @class VectorOrderProperty
   * @version Mar 22 2005
   * @author gromos++ development team
   * @sa utils::Property
   */
  class VectorOrderProperty : public Property
  {
  public:
    /**
     * Constructor.
     */
    VectorOrderProperty(gcore::System &sys, bound::Boundary * pbc);
    /**
     * Destructor.
     */
    virtual ~VectorOrderProperty();
    /**
     * Parse and check property specifier (given in arguments).
     * Calls Property::parse and checks the arguments.
     */
    virtual void parse(int count, std::string arguments[]);
    /**
     * Calculate the angle between the given atoms.
     */
    virtual double calc();
    /**
     * Calculate the average of all values calculated so far.
     */
    virtual std::string average();
    /**
     * Get a title string.
     */
    virtual std::string toTitle();
    /**
     * @struct Exception
     * AngleProperty exception.
     */
    struct Exception: public gromos::Exception
    {
      /**
       * Constructor.
       */
      Exception(const std::string &what): 
	gromos::Exception("VectorOrderProperty", what) {}
    };
      
  protected:
    /**
     * find the corresponding forcefield type of the property.
     * needs to be overwritten for the specific properties.
     */
    virtual int findTopologyType(gcore::MoleculeTopology const &mol_topo);

    /**
     * The average value.
     */
    double d_average;
    /**
     * The rmsd from the ideal value.
     */
    double d_zrmsd;    // <zero value> rmsd
    /**
     * Number of times calc() has been called.
     */
    int d_count;
    /**
     * vector 1
     */
    utils::VectorSpecifier d_vec1;
    /**
     * vector 1
     */
    utils::VectorSpecifier d_vec2;
    
  };

  /**
   * Class OrderParamProperty
   * Purpose: Implements an order parameter property.
   *
   * Description:
   * Implements an order parameter property
   * (order parameter of the angle between axis and vector specified
   * by two atoms).
   * It is derived from the Property class.
   * <br>
   * <span style="color:darkred;font-size:larger"><b>
   * @verbatim op%<AtomSpecifier>%zero_value%lower_bound%upper_bound @endverbatim
   * </b></span>
   * <br>
   *
   * @class OrderParamProperty
   * @version Jan 16 2004
   * @author gromos++ development team
   * @sa utils::Property
   */

  class OrderParamProperty : public Property
  {
  public:
    /**
     * Constructor.
     */
    OrderParamProperty(gcore::System &sys, bound::Boundary * pbc);
    /**
     * Destructor.
     */
    virtual ~OrderParamProperty();
    /**
     * Parse and check property specifier (given in arguments).
     * Calls Property::parse and checks the arguments.
     */
    virtual void parse(int count, std::string arguments[]);
    /**
     * Calculate the angle between the given atoms.
     */
    virtual double calc();
    /**
     * Calculate the average of all values calculated so far.
     */
    virtual std::string average();
    /**
     * Print the calculated value.
     */
    // virtual std::string toString();
    /**
     * Get a title string.
     */
    virtual std::string toTitle();
    /**
     * @struct Exception
     * AngleProperty exception.
     */
    struct Exception: public gromos::Exception
    {
      /**
       * Constructor.
       */
      Exception(const std::string &what): 
	gromos::Exception("OrderProperty", what) {}
    };
      
  protected:
    /**
     * find the corresponding forcefield type of the property.
     * needs to be overwritten for the specific properties.
     */
    virtual int findTopologyType(gcore::MoleculeTopology const &mol_topo);

    /**
     * The average value.
     */
    double d_average;
    /**
     * The rmsd from the ideal value.
     */
    double d_zrmsd;    // <zero value> rmsd
    /**
     * Number of times calc() has been called.
     */
    int d_count;
    /**
     * axis with respect to which the angle is calculated.
     */
    gmath::Vec d_axis;
      
  };

  /**
   * Class VectorOrderParamProperty
   * Purpose: Implements an order parameter property.
   *
   * Description:
   * Implements an order parameter property
   * (order parameter of the angle between two specified vectors)
   * It is derived from the Property class.
   * <br>
   * <span style="color:darkred;font-size:larger"><b>
   * @verbatim vop%<VectorSpecifier>%<VectorSpecifier> @endverbatim
   * </b></span>
   * <br>
   *
   * @class VectorOrderParamProperty
   * @version Jan 16 2004
   * @author gromos++ development team
   * @sa utils::Property
   */

  class VectorOrderParamProperty : public Property
  {
  public:
    /**
     * Constructor.
     */
    VectorOrderParamProperty(gcore::System &sys, bound::Boundary * pbc);
    /**
     * Destructor.
     */
    virtual ~VectorOrderParamProperty();
    /**
     * Parse and check property specifier (given in arguments).
     * Calls Property::parse and checks the arguments.
     */
    virtual void parse(int count, std::string arguments[]);
    /**
     * Calculate the angle between the given atoms.
     */
    virtual double calc();
    /**
     * Calculate the average of all values calculated so far.
     */
    virtual std::string average();
    /**
     * Get a title string.
     */
    virtual std::string toTitle();
    /**
     * @struct Exception
     * VectorOrderParamProperty exception.
     */
    struct Exception: public gromos::Exception
    {
      /**
       * Constructor.
       */
      Exception(const std::string &what): 
	gromos::Exception("VectorOrderParamProperty", what) {}
    };
      
  protected:
    /**
     * find the corresponding forcefield type of the property.
     * needs to be overwritten for the specific properties.
     */
    virtual int findTopologyType(gcore::MoleculeTopology const &mol_topo);

    /**
     * The average value.
     */
    double d_average;
    /**
     * The rmsd from the ideal value.
     */
    double d_zrmsd;    // <zero value> rmsd
    /**
     * Number of times calc() has been called.
     */
    int d_count;
    /**
     * vector 1
     */
    utils::VectorSpecifier d_vec1;
    /**
     * vector 1
     */
    utils::VectorSpecifier d_vec2;

  };

  inline double Property::getValue()
  {
    return d_value;
  }
  
  inline double Property::getZValue()
  {
    return d_zvalue;
  }
  
  inline double Property::getMinValue()
  {
    return d_lbound;
  }
  inline double Property::getMaxValue()
  {
    return d_ubound;
  }
  

  /*
    inline std::vector<int> Property::atoms()
    {
    return d_atom;
    }
  
    inline std::vector<int> Property::mols()
    {
    return d_mol;
    }
  */
  inline utils::AtomSpecifier & Property::atoms()
  {
    return d_atom;
  }

  /**
   * Class PseudoRotationProperty
   * Purpose: Implements a property that can calculate a pseudo rotation.
   *
   * Description:
   * This class implements a pseudo rotation. It is derived from the 
   * Property class. The pseudo rotation (\Delta / 2 ) is defined according  
   * to:
   * Altona, C; Geise, HJ; Romers C; Tetrahedron 24 13-32 (1968) 
   * With the addition that if \theta_0 < 0, then 180 is added to the value
   * For a (DNA)-sugar furanose ring this means that if one specifies the 
   * atoms for this property as C1',C2',C3',C4',O4' you can determine the
   * puckering, see:
   * Altona, C; Sundaralingam, M; JACS 94 8205-8212 (1972)
   * <br>
   * <span style="color:darkred;font-size:larger"><b>
   * @verbatim pr%<AtomSpecifier>%zero_value%lower_bound%upper_bound @endverbatim
   * </b></span>
   * <br>
   *
   * @class PseudoRotationProperty
   * @version Fri Apr 23 2004
   * @author gromos++ development team
   * @sa utils::Property
   */

  class PseudoRotationProperty : public Property
  {
  public:
    /**
     * Constructor.
     */
    PseudoRotationProperty(gcore::System &sys, bound::Boundary * pbc);
    /**
     * Destructor.
     */
    virtual ~PseudoRotationProperty();
    /**
     * Parse the arguments. Calls Property::parse.
     */
    virtual void parse(int count, std::string arguments[]);
    /**
     * Calculate the torsional angle.
     */
    virtual double calc();
    /**
     * Get the average of the calculated values.
     */
    virtual std::string average();
    /**
     * Get the value as string.
     */
    // virtual std::string toString();
    /**
     * Get a title string.
     */
    // virtual std::string toTitle();
    /**
     * @struct Exception
     * TorsionProperty exception.
     */
    struct Exception: public gromos::Exception
    {
      /**
       * Constructor.
       */
      Exception(const std::string &what): 
	gromos::Exception("PseudoRotationProperty", what) {}
    };
      
  protected:
    /**
     * find the corresponding forcefield type of the property.
     * needs to be overwritten for the specific properties.
     */
    virtual int findTopologyType(gcore::MoleculeTopology const &mol_topo);

    /**
     * The average value.
     */
    double d_average;
    /**
     * The rmsd from the ideal value.
     */
    double d_zrmsd;
    /**
     * Number of times calc() has been called.
     */
    int   d_count;
    /**
     * Function to calculate a torsion for four atoms
     */
    double _calcDihedral(int const a, int const b, int const c, int const d);
    /**
     * A constant that is needed every time
     * Should be sin(36) + sin(72);
     */
    double d_sin36sin72;
  };      

  /**
   * Class PuckerAmplitudeProperty
   * Purpose: Implements a property that can calculate a the amplitude of a 
   * pucker rotation.
   *
   * Description:
   * This class implements a pucker amplitude. It is derived from the 
   * PseudoRotationProperty class. The amplitude is defined according  
   * to:
   * Altona, C; Geise, HJ; Romers C; Tetrahedron 24 13-32 (1968) 
   * see also:
   * Altona, C; Sundaralingam, M; JACS 94 8205-8212 (1972)
   * <br>
   * <span style="color:darkred;font-size:larger"><b>
   * @verbatim pa%<AtomSpecifier>%zero_value%lower_bound%upper_bound @endverbatim
   * </b></span>
   * <br>
   *
   * @class PuckerAmplitudeProperty
   * @version Fri Apr 23 2004
   * @author gromos++ development team
   * @sa utils::PseudoRotationProperty
   * @sa utils::Property
   */

  class PuckerAmplitudeProperty : public PseudoRotationProperty
  {
  public:
    /**
     * Constructor.
     */
    PuckerAmplitudeProperty(gcore::System &sys, bound::Boundary * pbc);
    /**
     * Destructor.
     */
    virtual ~PuckerAmplitudeProperty();
    /**
     * Parse the arguments. Calls Property::parse.
     */
    virtual double calc();
    /**
     * Get a title string.
     */
    // virtual std::string toTitle();
  };    
}

#endif

