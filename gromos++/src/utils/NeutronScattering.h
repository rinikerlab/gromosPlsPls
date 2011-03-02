namespace utils {

  class iNS;

  /**
   * Class NS (Neutron Scattering) calculates the scattering intensities ...
   *
   * And some more descriptions here please...
   *
   * @class NS
   * @ingroup utils
   * @author A. Eichenberger
   */
  class NS {

  private:

    /**
     * A pointer to the implementation class containing the data
     */
    class iNS *d_this;
    /**
     * The default constructor: should not be used since it is likely
     * to forget the setting of the ponters and iterators by hand later on...
     */
    NS(void);

  public:

    /**
     * Constructor
     */
    NS(gcore::System *sys, args::Arguments::const_iterator firsttrj,
            args::Arguments::const_iterator lasttrj);

    /**
     * Destructor to delete the data (implementation class)
     */
    ~NS(void);

    /**
     * Sets the number of grid points of the resulting spectrum.
     * @param grid the number of grid points
     */
    void setGrid(int grid);
    /**
     * Sets the cutoff used in the RDF calculations
     * @param cut the number of grid points
     */
    void setCut(int cut);
    /**
     * Sets the maximum Q-value of the resulting spectrum.
     * @param Qmax the maximum Q-value
     */
    void setQmax(int Qmax);
    /**
     * Sets the atoms to be considered as centre atoms
     */
    int addAtoms(std::string s);
    /**
     * Gets all combination of centre to with atom types and resets the lengths
     * of the corresponding vectors to that length
     */
    int getCombinations(void);
    /**
     * Gets/sets the weighting (pre)factors for the inter- and intra-molecular
     * partial structure factors. Make sure the centre and with atoms are
     * set before using this function
     */
    void getWeights(void);
    /**
     * Checks the lengths and initialisation state of the members of NS to
     * test if it is ready for calculation of the scattering intensities
     */
    void check(void);
    /**
     * Sets a system variable to all dependant variables
     * @param sys a gromos system (gcore::System)
     */
    void setSystem(gcore::System *sys);
    /**
     * Set the iterators to the first and last trajectory file to be used
     * for the calculation.
     */
    void setTrajectories(args::Arguments::const_iterator firsttrj,
            args::Arguments::const_iterator lasttrj);
    /**
     * Set the centre and with atoms of all d_rdf members
     */
    void setRDFatoms();
    /**
     * Calculates all inter-molecular g(r) which are needed to build up the
     * structure factors S(Q) as well as the intensities I(Q)
     */
    void calcRDFsInterAll();
    /**
     * Prints the NEUTRONSCATTERING block
     */
    void print(std::ostream &os);

    void printRDFs(std::ostream &os);

  };

} /* end of namespace utils */
