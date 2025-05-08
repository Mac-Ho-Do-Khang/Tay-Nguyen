/*
 * Ho Chi Minh City University of Technology
 * Faculty of Computer Science and Engineering
 * Initial code for Assignment 2
 * Programming Fundamentals Spring 2025
 * Date: 02.02.2025
 */

// The library here is concretely set, students are not allowed to include any other libraries.
#ifndef _H_HCM_CAMPAIGN_H_
#define _H_HCM_CAMPAIGN_H_

#include "main.h"

////////////////////////////////////////////////////////////////////////
/// STUDENT'S ANSWER BEGINS HERE
/// Complete the following functions
/// DO NOT modify any parameters in the functions.
////////////////////////////////////////////////////////////////////////

struct Miscellanea
{
    static const char* NEWLINE;
    static const char* TAB;
    static const char* COMMA;
    static bool        DISPLAY_POWER_SUM ;
    static bool        DISPLAY_NODE_INDEX ;
    static bool        PRINT_MINIMAL_SUBSETs;
    static bool        PRINT_CONFIG_WHEN_HCMC_RUN;
    static string      LIBERATION_ARMY_NAME;
    static string      ARVN_NAME;
};

// Forward declaration
class Army;
class Position;
class Unit;
class UnitList;
class TerrainElement;
class BattleField;
class HCMCampaign;

class Vehicle;
class Infantry;

class LiberationArmy;
class ARVN;


class Road;
class Mountain;
class River;
class Urban;
class Fortification;
class SpecialZone;

class Configuration;

enum VehicleType
{
    TRUCK,
    MORTAR,
    ANTIAIRCRAFT,
    ARMOREDCAR,
    APC,
    ARTILLERY,
    TANK,
    NOT_A_VEHICLE
};
enum InfantryType
{
    SNIPER,
    ANTIAIRCRAFTSQUAD,
    MORTARSQUAD,
    ENGINEER,
    SPECIALFORCES,
    REGULARINFANTRY,
    NOT_AN_INFANTRY
};
enum class UnitType
{
    TRUCK,
    MORTAR,
    ANTIAIRCRAFT,
    ARMOREDCAR,
    APC,
    ARTILLERY,
    TANK,

    SNIPER,
    ANTIAIRCRAFTSQUAD,
    MORTARSQUAD,
    ENGINEER,
    SPECIALFORCES,
    REGULARINFANTRY,

    UNKNOWN
};

class Configuration
{
    friend class HCMCampaign; // access the 3 attributes below
    int num_rows;
    int num_cols;
    int eventCode;

public: // ----- to be deleted
    int liberation_unit_count, arvn_unit_count;

    vector<Position*> arrayForest, arrayRiver, arrayFortification, arrayUrban, arraySpecialZone;

public: // To be deleted ---
    Unit** liberationUnits; // Unit* -> Unit**
    Unit** ARVNUnits;       // Unit* -> Unit**
    // If use Unit*, then ARVNUnits[i] = Unit, slicing will occur to fit an Infantry/Vehicle to a Unit
    // Use Unit**, then ARVNUnits[i] = *Unit which can be either new Vehicle() or new Infantry(); we achieve dynamic binding via pointer

public:
    Configuration(const string& filepath);
    ~Configuration();
    string str() const;

    static VehicleType get_vehicle_type(const string&);
    static InfantryType get_infantry_type(const string&);
};

class Position
{
private:
    int r, c;

public:
    Position(int r = 0, int c = 0);
    Position(const string&); // Example: str_pos = "(1,15)"
    Position(const Position&);
    bool operator==(const Position&) const;

    int    getRow() const;
    int    getCol() const;
    void   setRow(int);
    void   setCol(int);
    string str() const; // Example: returns "(1,15)"
};

class Unit
{
    friend class UnitList;      // access to_unit_type()
    friend class Configuration; // Configuration::Configuration() access almost every attribute
    friend istringstream& operator>>(istringstream&, Unit*);

protected:
    int quantity, weight;
    Position pos;
    int armyBelonging;
    UnitType type;
    int attack_score; 
    virtual void initialize_attack_score_by_formula() = 0;
    static UnitType     to_unit_type(VehicleType);
    static UnitType     to_unit_type(InfantryType);
    static UnitType     to_unit_type(string);
    static string       to_string_type(UnitType);
    static InfantryType to_infantry_type(UnitType);
    static VehicleType  to_vehicle_type(UnitType);
    static bool         is_equal(Unit*, Unit*);
    // operator==(const Unit*, const Unit*) doesn't work as there must be at least 1 non-pointer parameter
public:
    Unit(int, int, const Position&, int = 0);
    Unit(Unit*);
    virtual int getAttackScore() = 0;
    virtual string str() const = 0;
    
    Position getCurrentPosition() const;
    UnitType get_unit_type() const;

    void setAttackScore(int);
    void increase_quantity_by(int);
    void set_weight(int);
    void set_quantity(int);
    int  get_weight();
    int  get_quantity();
    int  get_army_belonging();

    virtual ~Unit();
    bool operator==(const Unit&) const;
};

class Vehicle : public Unit
{
    VehicleType vehicleType;
    virtual void initialize_attack_score_by_formula() override;
public:
    Vehicle(int, int, const Position&, VehicleType, int = 0);
    Vehicle(Unit*);
    virtual int getAttackScore() override;
    virtual string str() const override;
};

class Infantry : public Unit
{
    InfantryType infantryType;
    virtual void initialize_attack_score_by_formula() override;
public:
    Infantry(int, int, const Position&, InfantryType, int = 0);
    Infantry(Unit*);
    virtual int getAttackScore() override;
    virtual string str() const override;
};

class Army
{
    friend class TerrainElement; // *unitList
    
    // Even though LiberationArmy is a derived class of Army, it cannot access protected members of other Army objects—only its own.
    // Same goes for ARVN
    friend class LiberationArmy; // LiberationArmy::fight() when accessing enemy->unitList
    friend class ARVN;

protected:
    int LF, EXP;
    string name;
    UnitList *unitList;
    BattleField *battleField;
    bool liberation;

    void calculate_LF();
    void calculate_EXP();

public:
    Army(Unit**, int, string, BattleField*);
    virtual bool fight(Army *enemy, bool defense = false) = 0; // void -> bool
    virtual string str() const = 0;
    void adjust_EXP(int);
    void adjust_LF(int);
    int get_EXP();
    int get_LF();
    bool is_liberation() const;
    ~Army();
};

class LiberationArmy : public Army
{
public:
    LiberationArmy(Unit**, int, string, BattleField*);
    virtual bool fight(Army*, bool = false) override;
    virtual string str() const override;
    static int next_fibonacci(int);
};

class ARVN : public Army
{
public:
    ARVN(Unit**, int, string, BattleField*);
    virtual bool fight(Army*, bool = false) override;
    virtual string str() const override;
};

class UnitList
{
    friend class Army;              // process_capacity()
    friend class LiberationArmy;    // first_unit, first_vehicle
    friend class ARVN;              // first_vehicle, last_unit
    // ------- for_each() -------
    friend class TerrainElement;
    friend class Road; 
    friend class Mountain; 
    friend class River; 
    friend class Urban; 
    friend class Fortification; 
    friend class SpecialZone; 

    class unit_node
    {
        friend class UnitList;
        friend class ARVN; // access index for removal

        int index; // Unique to distinguish each node (to remove later on), as attack score of `value` may be the same among nodes
        Unit* value;
        unit_node* next;
        unit_node(Unit*);
    };
    
    unit_node* first_unit, * last_unit, * first_vehicle;
    /*
    *       I -> I -> I -> V -> V -> V -> V -> nullptr
    *       ↑              ↑              ↑
    *   first_unit    first_vehicle   last_unit
    */
    int capacity, size, vehicle_count, infantry_count;
    static int node_index; // identify each unit_node for removal

    bool insert_first(Unit*);
    bool insert_last(Unit*);
    // Cannot use the function pointer version because the passed
    // lambda needs to capture [this] to access the member `unitList`
    template <typename Function> void for_each_infantry(Function);
    template <typename Function> void for_each_vehicle(Function);
    template <typename Function> void for_each_unit(Function);
    void clear_vehicles();   // remove all Vehicles
    void clear_infantries(); // remove all Infantries
    void clear();            // empty the list

    static int power_sum(int, int, int);
    static int power_sum_with_record(int, int, int, vector<int>&, vector<vector<int>>&);
    void process_capacity(int);

    // --------------- LiberationArmy::fight() ---------------
    void find_subsets(vector<vector<int>>&, int, vector<vector<int>>&,
                      vector<vector<vector<int>>>&, int, int&, int);
    /*
    * The array to consider is `scores_with_indices`
    * We only need one subset      -> vector<int>
                       (but which one if there are many ? )
    * So for now I record every possible subsets
                                   -> vector<vector<int>>
    * But along with the indices of the nodes to be removed, I also record their
    * corresponding attack scores  -> vector<vector<pair<int>>>
    * but cannot use pair<>        -> vector<vector<vector<int>>>
    */
    vector<vector<vector<int>>> // return empty vector if no valid subset found
        minimum_subsets(vector<vector<int>>&, int);
public:
    UnitList();
    bool insert(Unit*);
    bool remove(int); // Remove a node based on its index
    bool remove(unit_node*&);
    bool isContain(VehicleType) const; 
    bool isContain(InfantryType) const; 
    static Unit* get_node_data(unit_node*);
    static int get_node_index(unit_node*);
    static vector<vector<int>> to_vector(unit_node*, unit_node*);  // record attack scores_indices as vector
    string str() const;
    ~UnitList();
};

class TerrainElement
{
protected:
    Position pos;
    virtual void affect_infantry(Army*, UnitList::unit_node*) = 0;
    virtual void affect_vehicle(Army*, UnitList::unit_node*) = 0;

public:
    TerrainElement(Position);
    static int euclid_distance(const Position&, const Position&);
    ~TerrainElement();
    void getEffect(Army*);

    /*
    * Instead of having `getEffect` as pure virtual function, have `affect_infantry` and `affect_vehicle` as pure virtual ones.
    * `getEffect` simply calls UnitList::for_each_(infantry/vehicle) and pass the effect function
    * Traversing the linked list is handled by UnitList::for_each_(infantry/vehicle), not here
    * Each derived terrain implements their own effects
    */
};

class Road : public TerrainElement
{
    using TerrainElement::TerrainElement; // inherit constructor
    virtual void affect_infantry(Army*, UnitList::unit_node*) override;
    virtual void affect_vehicle(Army*, UnitList::unit_node*) override;
};

class Mountain : public TerrainElement
{
    using TerrainElement::TerrainElement; // inherit constructor
    virtual void affect_infantry(Army*, UnitList::unit_node*) override;
    virtual void affect_vehicle(Army*, UnitList::unit_node*) override;
};

class River : public TerrainElement
{
    using TerrainElement::TerrainElement; // inherit constructor
    virtual void affect_infantry(Army*, UnitList::unit_node*) override;
    virtual void affect_vehicle(Army*, UnitList::unit_node*) override;
};

class Urban : public TerrainElement
{
    using TerrainElement::TerrainElement; // inherit constructor
    virtual void affect_infantry(Army*, UnitList::unit_node*) override;
    virtual void affect_vehicle(Army*, UnitList::unit_node*) override;
};

class Fortification : public TerrainElement
{
    using TerrainElement::TerrainElement; // inherit constructor
    virtual void affect_infantry(Army*, UnitList::unit_node*) override;
    virtual void affect_vehicle(Army*, UnitList::unit_node*) override;
};

class SpecialZone : public TerrainElement
{
    using TerrainElement::TerrainElement; // inherit constructor
    virtual void affect_infantry(Army*, UnitList::unit_node*) override;
    virtual void affect_vehicle(Army*, UnitList::unit_node*) override;
};


class BattleField
{
    friend class HCMCampaign;
    int n_rows, n_cols;
    vector<vector<TerrainElement*>> terrain;
public:
    BattleField(int, int, vector<Position*>, vector<Position*>,
        vector<Position*>, vector<Position*>, vector<Position*>);
    ~BattleField();
};

class HCMCampaign
{
private:
    Configuration *config;
    BattleField *battleField;
    LiberationArmy *liberationArmy;
    ARVN *ARVNArmy;

public:
    HCMCampaign(const string &);
    ~HCMCampaign();
    void run();
    string printResult();
};

#endif