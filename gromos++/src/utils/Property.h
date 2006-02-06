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

#ifndef INCLUDED_UTILS_VALUE
#include "Value.h"
#endif

#ifndef INCLUDED_GMATH_STAT
#include "../gmath/Stat.h"
#endif

#ifndef INCLUDED_VECTOR_SPECIFIER
#include "VectorSpecifier.h"
#endif

#ifndef INCLUDED_GROMOS_EXCEPTION
#include "../gromos/Exception.h"
#endif

#ifndef INCLUDED_UTILS_EXPRESSIONPARSER
#include "ExpressionParser.h"
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
  std::ostream &operator<<(std::ostream &os, Property const & p);

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
   * - <b>expr</b> @ref ExpressionProperty "Expression Property"
   *
   * it's also possible to calculate the average (and rmsd) of a set of properties.
   * <span style="color:darkred;font-size:larger"><b>
   * @verbatim <d%1:1,2;d%1:5,9> @endverbatim
   * </b></span>
   * <br>
   * calculates the average and rmsd of the two distances d%1:1,2 and d%1:5,9.
   *
   * variable substitution
   * If 'a' is specified for the molecules, a property for each of the molecules is generated.
   * If 'a' is specified for an atom, all atoms of the molecule are taken.
   * Instead of 'a' also 'x' may be used. The values inserted for 'x' are specified as follows:
   * <span style="color:darkred;font-size:larger"><b>
   * @verbatim 
   @prop 'd%x:1,2|x=345,350-360'
   @prop 'd%1:(x),(x+1)|x=3,8'
   @endverbatim
   * </b></span>
   * <br>
   * The first will create distance properties (between atom 1 and 2) of molecules 345 and 350 to 360.
   * The second generates two properties, the first is d%1:3,4, the second d%1:8,9.
   * Note that the brackets are necessary around expressions (to distinguish a '-' sign from a range).
   *
   * The @ref AtomSpec "atom specifiers" should list the number of necessary atoms.
   * A <b>zero value</b> can be specified if deviation from this value is 
   * interesting.
   * If <b>lower</b> and <b>upper boundaries</b> are given, for any values outside the
   * boundaries, a message is printed to the output file
   * (for programs that activate this feature).
   * 
   * <b>See also</b> @ref AtomSpecifier "Atom Specifier" @ref VectorSpecifier "Vector Specifier"
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
     */
    Value const & getValue() { return d_value; }
    /**
     * Return the ideal (zero) value of the property.
     * @deprecated historic accessor, use args() instead
     */
    Value const & getZValue()
    { 
      if (d_arg.size()) return d_arg[0]; 
    }

    /**
     * arguments accessor
     */
    std::vector<Value> const & args() { return d_arg; }
    
    /**
     * As most of the properties i can think of have something to do with
     * atoms and molecules, i define these vectors in the base class.
     * This is also done in order to be able to write one general
     * arguments parsing function in the base class.
     */
    AtomSpecifier & atoms() { return d_atom; }
    /**
     * and a const variant
     */
    AtomSpecifier const & atoms()const { return d_atom; }
    /**
     * scalar stat
     */
    gmath::Stat<double> & getScalarStat() { return d_scalar_stat; }
    /**
     * vector stat
     */
    gmath::Stat<gmath::Vec> & getVectorStat(){ return d_vector_stat; }
    
    // methods
    
    /**
     * Calculates the value of the property.
     * Override in derived classes.
     */
    virtual Value const & calc() = 0;
    /**
     * Write a title string specifying the property.
     */
    virtual std::string toTitle()const;
    /**
     * Returns the value in string representation.
     */
    virtual std::string toString()const { return d_value.toString(); }
    /**
     * return the average, rmsd and error estimate of all calculations so far
     */
    virtual std::string average()const;    
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
    /**
     * Parse the command line property specification.
     * This is the standard implementation. It knows about
     * molecules, atoms, zero value and boundaries.
     * @TODO rewrite!!!
     */
    virtual void parse(std::vector<std::string> const & arguments, int x);
    /**
     * Property parser mainly written for the HB Property
     */
    virtual void parse(AtomSpecifier const & atmspc);

  protected:
    /**
     * Helper method to parse the atom part of the property specifier.
     * The argument is in AtomSpecifier format.
     */
    void parseAtoms(std::string atoms, int x);
    /**
     * find the corresponding forcefield type of the property.
     * needs to be overwritten for the specific properties.
     */
    virtual int findTopologyType(gcore::MoleculeTopology const &mol_topo);

    /**
     * add a value to the stats
     */
    void addValue(Value const & v);
    
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
     * The atoms belonging to this property.
     */
    AtomSpecifier d_atom;
    /**
     * the current value
     */
    Value d_value;
    /**
     * the ideal (zero) value
     */
    std::vector<Value> d_arg;
    /**
     * Reference of the system.
     */
    gcore::System *d_sys;
    /**
     * pointer to a boundary object
     */
    bound::Boundary *d_pbc;

    /**
     * Statistics of all calculated scalar values
     */
    gmath::Stat<double> d_scalar_stat;

    /**
     * Statistics of all calculated vector values
     */
    gmath::Stat<gmath::Vec> d_vector_stat;

  };


  ////////////////////////////////////////////////////////////////////////////////

  /**
   * Class AverageProperty
   * Purpose: Meta property that averages over a set of properties
   *
   * Description:
   * The AverageProperty class provides a 'meta-' property, a property
   * that contains a list of other properties and averages over those.
   * This one uses a list of ScalarProperties.
   *
   * @class AverageProperty
   * @version Tue Aug 23 2005
   * @author markus
   * @sa utils::Property
   */

  class AverageProperty : public Property
  {    
  public:
    /**
     * Constructor.
     */
    AverageProperty(gcore::System &sys, bound::Boundary * pbc);
    /**
     * Destructor.
     */
    virtual ~AverageProperty() {}
    /**
     * add a property
     */
    void addProperty(Property *p)
    {
      d_property.push_back(p);
    }
    /**
     * property accessor
     */
    Property * property(unsigned int i)
    {
      assert(i < d_property.size());
      return d_property[i];
    }
    /**
     * const property accessor
     */
    const Property * property(unsigned int i)const
    {
      assert(i < d_property.size());
      return d_property[i];
    }
    /**
     * properties accessor
     */
    std::vector<Property *> & properties()
    {
      return d_property;
    }
    /**
     * Calculate all properties.
     */
    virtual Value const & calc();
    /**
     * Write a title string specifying the property.
     */
    virtual std::string toTitle()const;
    /**
     * Returns the value in string representation.
     */
    virtual std::string toString()const;

  protected:
    /**
     * the properties to average over
     */
    std::vector<Property *> d_property;
    /**
     * single calc scalar statistics
     */
    gmath::Stat<double> d_single_scalar_stat;
    /**
     * single calc vector statistics
     */
    gmath::Stat<gmath::Vec> d_single_vector_stat;

  };

  ////////////////////////////////////////////////////////////////////////////////

  /**
   * Class DistributionProperty
   * Purpose: Meta property that averages over a set of properties
   *
   * Description:
   * The DistributionProperty class provides a 'meta-' property, a property
   * that contains a list of other properties and averages over those.
   * This one uses a list of ScalarProperties.
   *
   * @class DistributionProperty
   * @version Tue Aug 23 2005
   * @author markus
   * @sa utils::Property
   */

  class DistributionProperty : public Property
  {    
  public:
    /**
     * Constructor.
     */
    DistributionProperty(gcore::System &sys, bound::Boundary * pbc);
    /**
     * Destructor.
     */
    virtual ~DistributionProperty() {}
    /**
     * add a property
     */
    void addProperty(Property *p)
    {
      d_property.push_back(p);
    }
    /**
     * property accessor
     */
    Property * property(unsigned int i)
    {
      assert(i < d_property.size());
      return d_property[i];
    }
    /**
     * const property accessor
     */
    const Property * property(unsigned int i)const
    {
      assert(i < d_property.size());
      return d_property[i];
    }
    /**
     * properties accessor
     */
    std::vector<Property *> & properties()
    {
      return d_property;
    }
    /**
     * Calculate all properties.
     */
    virtual Value const & calc();
    /**
     * Write a title string specifying the property.
     */
    virtual std::string toTitle()const;
    /**
     * Returns the value in string representation.
     */
    virtual std::string toString()const;

  protected:
    /**
     * the properties to average over
     */
    std::vector<Property *> d_property;
    /**
     * single calc scalar statistics
     */
    gmath::Stat<double> d_single_scalar_stat;
    /**
     * single calc vector statistics
     */
    gmath::Stat<gmath::Vec> d_single_vector_stat;

  };

  ////////////////////////////////////////////////////////////////////////////////

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
    virtual ~DistanceProperty() {}
    
    /**
     * Parses the arguments. Is overridden to check the input, but basically
     * just calls Property::parse.
     * @sa utils::Property::parse
     */
    virtual void parse(std::vector<std::string> const & arguments, int x);
    /**
     * Property parser mainly written for the HB Property
     */
    virtual void parse(AtomSpecifier const & atmspc);
    /**
     * Calculates the distance.
     */
    virtual Value const & calc();

  protected:

    /**
     * find the corresponding forcefield type of the property.
     * needs to be overwritten for the specific properties.
     */
    virtual int findTopologyType(gcore::MoleculeTopology const &mol_topo);
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
    virtual ~AngleProperty() {}
    
    /**
     * Parse and check property specifier (given in arguments).
     * Calls Property::parse and checks the arguments.
     */
    virtual void parse(std::vector<std::string> const & arguments, int x);
    /**
     * Property parser mainly written for the HB Property
     */
    virtual void parse(AtomSpecifier const & atmspc);
    /**
     * Calculate the angle between the given atoms.
     */
    virtual Value const & calc();
      
  protected:
    /**
     * find the corresponding forcefield type of the property.
     * needs to be overwritten for the specific properties.
     */
    virtual int findTopologyType(gcore::MoleculeTopology const &mol_topo);
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
    virtual void parse(std::vector<std::string> const & arguments, int x);
    /**
     * Property parser mainly written for the HB Property
     */
    virtual void parse(AtomSpecifier const & atmspc);
    /**
     * Calculate the torsional angle.
     */
    virtual Value const & calc();
  protected:
    /**
     * find the corresponding forcefield type of the property.
     * needs to be overwritten for the specific properties.
     */
    virtual int findTopologyType(gcore::MoleculeTopology const &mol_topo);
  };    

  /**
   * Class JValueProperty
   * Purpose: Implements a J-value property.
   *
   * Description:
   * This class implements a J-value property. It is derived from the 
   * Property class.
   * <br>
   * <span style="color:darkred;font-size:larger"><b>
   * @verbatim t%<AtomSpecifier>%zero_value%lower_bound%upper_bound @endverbatim
   * </b></span>
   * <br>
   *
   * @class JValueProperty
   * @version Fri Dec 23 2005
   * @author gromos++ development team
   * @sa utils::Property
   */

  class JValueProperty : public Property
  {
  public:
    /**
     * Constructor.
     */
    JValueProperty(gcore::System &sys, bound::Boundary * pbc);
    /**
     * Destructor.
     */
    virtual ~JValueProperty();
    /**
     * Parse the arguments. Calls Property::parse.
     */
    virtual void parse(std::vector<std::string> const & arguments, int x);
    /**
     * Calculate the J-value of a torsional angle.
     */
    virtual Value const & calc();
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
    virtual void parse(std::vector<std::string> const & arguments, int x);
    /**
     * Calculate the angle between the given atoms.
     */
    virtual Value const & calc();
    /**
     * Write a title string specifying the property.
     */
    virtual std::string toTitle()const;

  protected:
    /**
     * find the corresponding forcefield type of the property.
     * needs to be overwritten for the specific properties.
     */
    virtual int findTopologyType(gcore::MoleculeTopology const &mol_topo);
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
    virtual void parse(std::vector<std::string> const & arguments, int x);
    /**
     * Calculate the angle between the given atoms.
     */
    virtual Value const & calc();
    /**
     * Write a title string specifying the property.
     */
    virtual std::string toTitle()const;

  protected:
    /**
     * find the corresponding forcefield type of the property.
     * needs to be overwritten for the specific properties.
     */
    virtual int findTopologyType(gcore::MoleculeTopology const &mol_topo);

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
    virtual void parse(std::vector<std::string> const & arguments, int x);
    /**
     * Calculate the angle between the given atoms.
     */
    virtual Value const & calc();
    /**
     * Write a title string specifying the property.
     */
    virtual std::string toTitle()const;

  protected:
    /**
     * find the corresponding forcefield type of the property.
     * needs to be overwritten for the specific properties.
     */
    virtual int findTopologyType(gcore::MoleculeTopology const &mol_topo);
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
    virtual void parse(std::vector<std::string> const & arguments, int x);
    /**
     * Calculate the angle between the given atoms.
     */
    virtual Value const & calc();
    /**
     * Write a title string specifying the property.
     */
    virtual std::string toTitle()const;

  protected:
    /**
     * find the corresponding forcefield type of the property.
     * needs to be overwritten for the specific properties.
     */
    virtual int findTopologyType(gcore::MoleculeTopology const &mol_topo);

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
    virtual void parse(std::vector<std::string> const & arguments, int x);
    /**
     * Calculate the torsional angle.
     */
    virtual Value const & calc();

  protected:
    /**
     * find the corresponding forcefield type of the property.
     * needs to be overwritten for the specific properties.
     */
    virtual int findTopologyType(gcore::MoleculeTopology const &mol_topo);

    /**
     * Function to calculate a torsion for four atoms
     */
    double _calcDihedral(int const a, int const b, int const c, int const d);
    /**
     * A constant that is needed every time
     * Should be sin(36) + sin(72);
     */
    const double d_sin36sin72;
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
    virtual Value const & calc();
  };    

  /**
   * Class ExpressionProperty
   * Purpose: Implements an expression property
   *
   * Description:
   * The expression property allows the evaluation of a multidude of expressions
   * over a trajectory.
   * The general form is:
   * <br>
   * <span style="color:darkred;font-size:larger"><b>
   * @verbatim expr%f1() op f2() @endverbatim
   * </b></span>
   * <br>
   * where op is an operator ( + - * / )
   * and where f1() and f2() are functions
   * (sin, cos, tan, asin, acos, atan, exp, ln, abs, abs2, sqrt, dot, cross, ni (nearest image))
   * (and probably some more).
   * the arguments are @ref VectorSpecifier "vector specifiers".
   * 
   * example:
   * <br>
   * <span style="color:darkred;font-size:larger"><b>
   * @verbatim expr%dot(atom(1:1),cart(0,0,1)) @endverbatim
   * </b></span>
   * <br>
   * calculates the dot product between position of atom(1:1) and
   * the vector (0,0,1); that is the z-component of the position
   * of the first atom of the first molecule.
   *
   * @version Aug 25 2005
   * @author markus
   * @sa utils::Property
   */
  class ExpressionProperty : public Property
  {
  public:
    /**
     * Constructor.
     */
    ExpressionProperty(gcore::System &sys, bound::Boundary * pbc);
    /**
     * Destructor.
     */
    virtual ~ExpressionProperty();
    /**
     * Parse and check property specifier (given in arguments).
     * Calls Property::parse and checks the arguments.
     */
    virtual void parse(std::vector<std::string> const & arguments, int x);
    /**
     * Calculate the angle between the given atoms.
     */
    virtual Value const & calc();
    /**
     * Write a title string specifying the property.
     */
    virtual std::string toTitle()const;

  protected:
    /**
     * find the corresponding forcefield type of the property.
     * needs to be overwritten for the specific properties.
     */
    virtual int findTopologyType(gcore::MoleculeTopology const &mol_topo);

    /**
     * calculates expressions
     */
    ExpressionParser<Value> d_parser;
    
    /**
     * stores the expression
     */
    std::vector<ExpressionParser<Value>::expr_struct> d_expr;
    
    /**
     * could store variables
     */
    std::map<std::string, Value> d_var;

  };

  /**
   * Class HBProperty
   * Purpose: Implements a HB property.
   *
   * Description:
   * This class implements a HB property. It is derived from the 
   * Property class.
   * <br>
   * <span style="color:darkred;font-size:larger"><b>
   * @verbatim t%<AtomSpecifier>%distance_upper_bound%angle_lower_bound @endverbatim
   * </b></span>
   * <br>
   *
   * @class HBProperty
   * @version Mon Oct 31 2005
   * @author gromos++ development team
   * @sa utils::Property
   */

  class HBProperty : public Property
    {
  public:
    /**
     * Constructor.
     */
    HBProperty(gcore::System &sys, bound::Boundary * pbc);
    /**
     * Destructor.
     */
    virtual ~HBProperty();
    /**
     * Parse the arguments. Calls Property::parse.
     */
    virtual void parse(std::vector<std::string> const & arguments, int x);
    /**
     * Calculates the HB.
     */
    virtual Value const & calc();
   /**
    * Returns the value in string representation.
    */
    virtual std::string toTitle()const;

  protected:
    /**
    * find the type of the property.
    * is case of HB: return 1 if 3center or 0 for no 3c 
    */
    virtual int findTopologyType(gcore::MoleculeTopology const &mol_topo);
    // member variables
    /** HB distance (2 if 3 center)
     */
    DistanceProperty d1_hb;
    DistanceProperty d2_hb;
    /** HB angles (3 if 3 center)
     */
    AngleProperty a1_hb;
    AngleProperty a2_hb;
    AngleProperty a3_hb;
    /** HB improper (1 if 3 center)
     */
    TorsionProperty i1_hb;
    /** test whether it is a HB (and 3cHB)
     */
    bool ishb;
    bool is3c;  
    /** auxiliary atoms specifier for HB
     */
    AtomSpecifier as;
    };

}

#endif

