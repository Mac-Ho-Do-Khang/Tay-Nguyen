#include "hcmcampaign.h"

/*
* Giả sử ảnh hưởng của địa hình lên EXP là n
    - ceil(n) rồi EXP+-n  (hiện đang làm cách này)
    - hay ceil(EXP+-n)? 
* Trong thế attack của LiberationArmy::fight(), nếu có nhiều hơn 1 tổ hợp A/B thỏa mãn thì sao?
* Khi insert thêm Unit vào UnitList, xác định một Unit đã có sẵn dựa trên các yếu tố nào? (Loại, vị trí, weight)?
* Giả sử có 10 unit, tính ra được EXP và LF, từ đó tính ra S, tính ra capacity=8 -> lấy 8/10 units 
    - có tính lại EXP' và LF' trên 8 unit này không? 
    - 2 unit còn lại có để dành để thêm vào sau khi một số unit trong unitList bị bỏ bớt ra không?
* LA tấn công == ARVN phòng thủ và ngược lại ???
    */


// ------------------------- Miscellanea -------------------------
const char* Miscellanea::NEWLINE{ "\n" };
const char* Miscellanea::COMMA{ "\n" };
const char* Miscellanea::TAB{ "\t" };
bool        Miscellanea::DISPLAY_POWER_SUM{ true };
bool        Miscellanea::DISPLAY_NODE_INDEX{ true };
bool        Miscellanea::PRINT_MINIMAL_SUBSETs{ true };
bool        Miscellanea::PRINT_CONFIG_WHEN_HCMC_RUN{ true };
string      Miscellanea::LIBERATION_ARMY_NAME{ };
string      Miscellanea::ARVN_NAME{ };

//const char* Miscellanea::NEWLINE{ "" };
//const char* Miscellanea::COMMA{ "," };
//const char* Miscellanea::TAB{ "" };
//bool        Miscellanea::DISPLAY_POWER_SUM{ false };
//bool        Miscellanea::DISPLAY_NODE_INDEX{ false };
//bool        Miscellanea::PRINT_MINIMAL_SUBSETs{ false };
//bool        Miscellanea::PRINT_CONFIG_WHEN_HCMC_RUN{ true };
//string      Miscellanea::LIBERATION_ARMY_NAME{ };
//string      Miscellanea::ARVN_NAME{ };

istringstream& operator>>(istringstream& is, int& n)
{
    char c{};
    while (is.peek() != EOF && (is.peek() < '0' || is.peek() > '9')) is.get(c);
    (istream&)is >> n;

    return is;
}

istringstream& operator>>(istringstream& is, Position& p) 
{
    char ch{};
    while (is.get(ch)) // Consume characters until '('
        if (ch == '(') break;
    int r{}, c{};
    is >> r >> c;
    p.setRow(r); p.setCol(c);
    return is;
}

istringstream& operator>>(istringstream& is, Unit* u)
{
    char ch{}; string str; bool first_bracket{ true };
    while (is.get(ch)) // Consume characters until ')'
    {
        str.push_back(ch);
        if (ch == ')')
            if (!first_bracket) break;
            else first_bracket = false;
    }
    //                 1st 2nd
    //                  ↓  ↓
    //.....TANK(5,2,(1,2),0)
    //     ↑   ↑
    //     p1  p2

    // Extract type
    size_t p1{ str.find_first_of("AEMRSTU") };
    size_t p2{ str.find('(') };
    if (p1 != string::npos && p2 != string::npos)
    {
        string str_type{ str.begin() + p1,str.begin() + p2 };
        u->type = Unit::to_unit_type(str_type);
    }
    

    // Extract quantity, weight, pos, armyBelonging
    istringstream iss{ str };
    iss >> u->quantity >> u->weight >> u->pos >> u->armyBelonging;

    return is;
}

ostringstream& operator<<(ostringstream& os, const InfantryType& i)
{
    switch (i)
    {
        case SNIPER             : os << "SNIPER";            break;
        case ANTIAIRCRAFTSQUAD  : os << "ANTIAIRCRAFTSQUAD"; break;
        case MORTARSQUAD        : os << "MORTARSQUAD";       break;
        case ENGINEER           : os << "ENGINEER";          break;
        case SPECIALFORCES      : os << "SPECIALFORCES";     break;
        case REGULARINFANTRY    : os << "REGULARINFANTRY";   break;
        default                 : os << "Unknown Infantry type";
    }
    return os;
}

ostringstream& operator<<(ostringstream& os, const VehicleType& v)
{
    switch (v)
    {
        case TRUCK          : os << "TRUCK";        break;
        case MORTAR         : os << "MORTAR";       break;
        case ANTIAIRCRAFT   : os << "ANTIAIRCRAFT"; break;
        case ARMOREDCAR     : os << "ARMOREDCAR";   break;
        case APC            : os << "APC";          break;
        case ARTILLERY      : os << "ARTILLERY";    break;
        case TANK           : os << "TANK";         break;
        default             : os << "Unknown Vehicle type";
    }
    return os;
}

ostringstream& operator<<(ostringstream& os, const Position& p)
{
    os << p.str().c_str(); // no operator<<(ostringstream&, string) defined
    return os;
}

ostringstream& operator<<(ostringstream& os, const Position* p)
{
    os << p->str().c_str(); // no operator<<(ostringstream&, string) defined
    return os;
}

//                                   .---------------.
// ----------------------------------| CONFIGURATION |----------------------------------
//                                   '---------------'
Configuration::Configuration(const string& filepath)
    : num_rows{}, num_cols{}, eventCode{}, liberation_unit_count{}, arvn_unit_count{}, liberationUnits{}, ARVNUnits{}
{
    fstream file{ filepath,ios::in };
    if (!file) 
    {
        cerr << "Error: Cannot open file: " << filepath << endl;
        exit(1);
    }
    vector<string> unit_lines;
    string line;
    while (getline(file, line))
    {
        istringstream iss{ line };
        Position p{};
        string s;
        if (line.find("NUM_ROWS") != string::npos)        iss >> num_rows;
        else if (line.find("NUM_COLS") != string::npos)   iss >> num_cols;
        else if (line.find("EVENT_CODE") != string::npos) 
        {
            iss >> eventCode;
            if (eventCode < 0) eventCode = 0;
            else eventCode %= 100; // only take the last 2 digits when > 100
        }
        else if (line.find("ARRAY_FOREST") != string::npos) 
            while (iss >> p)
                arrayForest.push_back(new Position{ p });
        else if (line.find("ARRAY_RIVER") != string::npos) 
            while (iss >> p)
                arrayRiver.push_back(new Position{ p });
        else if (line.find("ARRAY_FORTIFICATION") != string::npos) 
            while (iss >> p)
                arrayFortification.push_back(new Position{ p });
        else if (line.find("ARRAY_URBAN") != string::npos) 
            while (iss >> p)
                arrayUrban.push_back(new Position{ p });
        else if (line.find("ARRAY_SPECIAL_ZONE") != string::npos) 
            while (iss >> p)
                arraySpecialZone.push_back(new Position{ p });
        else 
        {
            // -------- Count the number of liberation/arvn units
            int n{}, x{};
            istringstream is{ line };
            while (is >> x)
            {
                n++;
                // The digits marking to which a unit belongs are the 5th, 10h, 15th... ones (1-based)
                if (n % 5 == 0) 
                    if (x == 0) liberation_unit_count++;
                    else        arvn_unit_count++;
            }
            liberationUnits = new Unit * [liberation_unit_count];
            ARVNUnits = new Unit * [arvn_unit_count];
                        
            // UNIT_LIST=[TANK(5,2,(1,2),0),...]
            //           ↑                     ↑
            //     line.begin() + p        line.end()
            size_t p{ line.find_first_of("[") };
            string units(line.begin() + p, line.end()); // Remove "UNIT_LIST=["

            Unit* u = new Infantry{ 0,0,Position{}, NOT_AN_INFANTRY };
            istringstream iss{ units };
            int liberation_index{}, arvn_index{};
            while (iss >> u)
            {
                if (u->armyBelonging == 0) // Liberation's unit
                {
                    if (Configuration::get_vehicle_type(Unit::to_string_type(u->type)) != NOT_A_VEHICLE)
                        liberationUnits[liberation_index++] = new Vehicle{ u };
                    else
                        liberationUnits[liberation_index++] = new Infantry{ u };
                }
                else        // ARVN's unit
                {
                    if (Configuration::get_vehicle_type(Unit::to_string_type(u->type)) != NOT_A_VEHICLE)
                        ARVNUnits[arvn_index++] = new Vehicle{ u };
                    else
                        ARVNUnits[arvn_index++] = new Infantry{ u };
                }
            }
            delete u;
        }
    }
    file.close();
}
VehicleType  Configuration::get_vehicle_type(const string& s)
{
    if      (s == "TRUCK")          return TRUCK;
    else if (s == "MORTAR")         return MORTAR;
    else if (s == "ANTIAIRCRAFT")   return ANTIAIRCRAFT;
    else if (s == "ARMOREDCAR")     return ARMOREDCAR;
    else if (s == "APC")            return APC;
    else if (s == "ARTILLERY")      return ARTILLERY;
    else if (s == "TANK")           return TANK;
    else                            return NOT_A_VEHICLE;
}
InfantryType Configuration::get_infantry_type(const string& s)
{
    if      (s == "SNIPER")             return SNIPER;
    else if (s == "ANTIAIRCRAFTSQUAD")  return ANTIAIRCRAFTSQUAD;
    else if (s == "MORTARSQUAD")        return MORTARSQUAD;
    else if (s == "ENGINEER")           return ENGINEER;
    else if (s == "SPECIALFORCES")      return SPECIALFORCES;
    else if (s == "REGULARINFANTRY")    return REGULARINFANTRY;
    else                                return NOT_AN_INFANTRY;
}
string       Configuration::str() const
{
    ostringstream oss{};
    oss << "[" << Miscellanea::NEWLINE;
    oss << "num_rows=" << num_rows << Miscellanea::COMMA;
    oss << "num_cols=" << num_cols << Miscellanea::COMMA;

    oss << "arrayForest=[";
    for (const Position* i : arrayForest) oss << i << ",";
    oss.seekp(-1, ios_base::end); // Remove the trailing ','
    oss << "]" << Miscellanea::COMMA;

    oss << "arrayRiver=[";
    for (const Position* i : arrayRiver) oss << i << ",";
    oss.seekp(-1, ios_base::end); // Remove the trailing ','
    oss << "]" << Miscellanea::COMMA;

    oss << "arrayFortification=[";
    for (const Position* i : arrayFortification) oss << i << ",";
    oss.seekp(-1, ios_base::end); // Remove the trailing ','
    oss << "]" << Miscellanea::COMMA;

    oss << "arrayUrban=[";
    for (const Position* i : arrayUrban) oss << i << ",";
    oss.seekp(-1, ios_base::end); // Remove the trailing ','
    oss << "]" << Miscellanea::COMMA;

    oss << "arraySpecialZone=[";
    for (const Position* i : arraySpecialZone) oss << i << ",";
    oss.seekp(-1, ios_base::end); // Remove the trailing ','
    oss << "]" << Miscellanea::COMMA;

    oss << "liberationUnits=[" << Miscellanea::NEWLINE;
    for (int i{}; i < liberation_unit_count; ++i)
        oss << liberationUnits[i]->str().c_str() << Miscellanea::COMMA;
    oss.seekp(-1, ios_base::end); // Remove the trailing ','
    oss << "]" << Miscellanea::COMMA;

    oss << "ARVNUnits=[" << Miscellanea::NEWLINE;
    for (int i{}; i < arvn_unit_count; ++i)
        oss << ARVNUnits[i]->str().c_str() << Miscellanea::COMMA;
    oss.seekp(-1, ios_base::end); // Remove the trailing ','
    oss << "]" << Miscellanea::COMMA;

    oss << "eventCode=" << eventCode << Miscellanea::NEWLINE << "]";

    return oss.str();
}
Configuration::~Configuration()
{
    for (Position* p : arrayForest) delete p;
    for (Position* p : arrayRiver) delete p;
    for (Position* p : arrayFortification) delete p;
    for (Position* p : arrayUrban) delete p;
    for (Position* p : arraySpecialZone) delete p;

    for (int i{}; i < liberation_unit_count; ++i)
        delete liberationUnits[i];
    delete[] liberationUnits;
    for (int i{}; i < arvn_unit_count; ++i)
        delete ARVNUnits[i];
    delete[] ARVNUnits;
}

//                                   .----------.
// ----------------------------------| POSITION |----------------------------------
//                                   '----------'
Position::Position(int r, int c)          : r{ r }, c{ c } {}
Position::Position(const Position& other) : Position{ other.getRow(),other.getCol() } {}
Position::Position(const string& str_pos)
{
    istringstream iss{ str_pos };
    iss >> r >> c;
}
int    Position::getRow() const { return r; }
int    Position::getCol() const { return c; }
void   Position::setRow(int n) { r = n; }
void   Position::setCol(int n) { c = n; }
string Position::str() const
{
    return string{ "(" }.append(to_string(r)).append(",").append(to_string(c)).append(")");
}
bool   Position::operator==(const Position& other) const
{
    return r == other.r && c == other.c;
}

//                                   .------.
// ----------------------------------| UNIT |----------------------------------
//                                   '------'
Unit::Unit(int quantity, int weight, const Position& pos, int armyBelonging) :
    quantity{ quantity }, weight{ weight }, pos{ pos }, armyBelonging{ armyBelonging }, attack_score{}, type{ UnitType::UNKNOWN } {}
Unit::Unit(Unit* other) 
    : Unit{ other->quantity,other->weight,other->pos,other->armyBelonging } 
{
    type = other->type;
}
Position Unit::getCurrentPosition() const { return pos; }
void Unit::increase_quantity_by(int n)    { quantity += n; }
void Unit::setAttackScore(int i)          { attack_score = i; }
void Unit::set_weight(int i)              { weight = i; };
int  Unit::get_weight()                   { return weight; };
void Unit::set_quantity(int i)            { quantity = i; };
int  Unit::get_quantity()                 { return quantity; };
int  Unit::get_army_belonging()           { return armyBelonging; };
UnitType     Unit::get_unit_type() const  { return type; }
UnitType     Unit::to_unit_type(VehicleType t)
{
    switch (t)
    {
        case TRUCK          : return UnitType::TRUCK;
        case MORTAR         : return UnitType::MORTAR;
        case ANTIAIRCRAFT   : return UnitType::ANTIAIRCRAFT;
        case ARMOREDCAR     : return UnitType::ARMOREDCAR;
        case APC            : return UnitType::APC;
        case ARTILLERY      : return UnitType::ARTILLERY;
        default             : return UnitType::TANK;
    }
}
UnitType     Unit::to_unit_type(InfantryType t)
{
    switch (t)
    {
        case SNIPER             : return UnitType::SNIPER;
        case ANTIAIRCRAFTSQUAD  : return UnitType::ANTIAIRCRAFTSQUAD;
        case MORTARSQUAD        : return UnitType::MORTARSQUAD;
        case ENGINEER           : return UnitType::ENGINEER;
        case SPECIALFORCES      : return UnitType::SPECIALFORCES;
        default                 : return UnitType::REGULARINFANTRY;
    }
}
UnitType     Unit::to_unit_type(string s)
{
    if      (s == "TRUCK")              return UnitType::TRUCK;
    else if (s == "MORTAR")             return UnitType::MORTAR;
    else if (s == "ANTIAIRCRAFT")       return UnitType::ANTIAIRCRAFT;
    else if (s == "ARMOREDCAR")         return UnitType::ARMOREDCAR;
    else if (s == "APC")                return UnitType::APC;
    else if (s == "ARTILLERY")          return UnitType::ARTILLERY;
    else if (s == "TANK")               return UnitType::TANK;

    else if (s == "SNIPER")             return UnitType::SNIPER;
    else if (s == "ANTIAIRCRAFTSQUAD")  return UnitType::ANTIAIRCRAFTSQUAD;
    else if (s == "MORTARSQUAD")        return UnitType::MORTARSQUAD;
    else if (s == "ENGINEER")           return UnitType::ENGINEER;
    else if (s == "SPECIALFORCES")      return UnitType::SPECIALFORCES;
    else if (s == "REGULARINFANTRY")    return UnitType::REGULARINFANTRY;

    return UnitType::UNKNOWN;
}
string       Unit::to_string_type(UnitType t)
{
    switch (t)
    {
        case UnitType::TRUCK                : return "TRUCK";
        case UnitType::MORTAR               : return "MORTAR";
        case UnitType::ANTIAIRCRAFT         : return "ANTIAIRCRAFT";
        case UnitType::ARMOREDCAR           : return "ARMOREDCAR";
        case UnitType::APC                  : return "APC";
        case UnitType::ARTILLERY            : return "ARTILLERY";
        case UnitType::TANK                 : return "TANK";
        case UnitType::SNIPER               : return "SNIPER";
        case UnitType::ANTIAIRCRAFTSQUAD    : return "ANTIAIRCRAFTSQUAD";
        case UnitType::MORTARSQUAD          : return "MORTARSQUAD";
        case UnitType::ENGINEER             : return "ENGINEER";
        case UnitType::SPECIALFORCES        : return "SPECIALFORCES";
        case UnitType::REGULARINFANTRY      : return "REGULARINFANTRY";
        default                             : return "UNKNOWN";
    }
}
InfantryType Unit::to_infantry_type(UnitType t)
{
    switch (t)
    {
        case UnitType::SNIPER:              return SNIPER;
        case UnitType::ANTIAIRCRAFTSQUAD:   return ANTIAIRCRAFTSQUAD;
        case UnitType::MORTARSQUAD:         return MORTARSQUAD;
        case UnitType::ENGINEER:            return ENGINEER;
        case UnitType::SPECIALFORCES:       return SPECIALFORCES;
        case UnitType::REGULARINFANTRY:     return REGULARINFANTRY;
        default:                            return NOT_AN_INFANTRY;
    }
}
VehicleType  Unit::to_vehicle_type(UnitType t)
{
    switch (t)
    {
        case UnitType::TRUCK:           return TRUCK;
        case UnitType::MORTAR:          return MORTAR;
        case UnitType::ANTIAIRCRAFT:    return ANTIAIRCRAFT;
        case UnitType::ARMOREDCAR:      return ARMOREDCAR;
        case UnitType::APC:             return APC;
        case UnitType::ARTILLERY:       return ARTILLERY;
        case UnitType::TANK:            return TANK;
        default:                        return NOT_A_VEHICLE;
    }
}
bool Unit::is_equal(Unit* lhs, Unit* rhs)
{
    return *lhs == *rhs;
    //if (auto* u = dynamic_cast<const Vehicle*>(lhs))
    //{
    //    if (auto* v = dynamic_cast<const Vehicle*>(rhs))
    //        return v->get_unit_type() == u->get_unit_type();
    //}
    //else if (auto* u = dynamic_cast<const Infantry*>(lhs))
    //{
    //    if (auto* v = dynamic_cast<const Infantry*>(rhs))
    //        return v->get_unit_type() == u->get_unit_type();
    //}
}
bool Unit::operator==(const Unit& other) const
{
    // For now, 2 Units are compared base on `type` and `pos` and `weight`
    return this->type == other.type && this->pos == other.pos && this->weight == other.weight;
}
Unit::~Unit() {}

//                                   .----------.
// ----------------------------------| INFANTRY |----------------------------------
//                                   '----------'
Infantry::Infantry(int quantity, int weight, const Position& pos, InfantryType infantryType, int armyBelonging)
    : Unit{ quantity,weight,pos,armyBelonging }, infantryType{ infantryType } 
{
    type = Unit::to_unit_type(infantryType);
    initialize_attack_score_by_formula();
}
Infantry::Infantry(Unit* u) :
    Infantry{ u->get_quantity(),u->get_weight(),u->getCurrentPosition(),
    Unit::to_infantry_type(u->get_unit_type()),
    u->get_army_belonging() } {}
void     Infantry::initialize_attack_score_by_formula()
{
    attack_score = infantryType * 56 + quantity * weight;
    bool (*is_perfect_square)(int) = [](int n) // Kiểm tra có là số chính phương
    {
        if (n < 0) return false;
        int root = (int)sqrt(n);
        return root * root == n;
    };
    if (infantryType == SPECIALFORCES && is_perfect_square(weight)) attack_score += 75;

    // Personal number stuffs
    int personal_number{ INT_MAX };
    for (string s_score{ to_string(attack_score).append("1975") }; personal_number >= 10; s_score = to_string(personal_number))
    {
        personal_number = 0;
        istringstream iss{ s_score };
        char c{};
        while (iss.get(c)) personal_number += (c - '0');
    }
    if (personal_number > 7) quantity = ceil(quantity * 1.2);
    else if (personal_number < 3) quantity = ceil(quantity * 0.9);
    attack_score = infantryType * 56 + quantity * weight; // Re-calculate score
}
int      Infantry::getAttackScore()
{
    return attack_score;
}
string   Infantry::str() const
{
    ostringstream oss{};
    oss << Miscellanea::TAB << "Infantry[";
    //oss << "infantryType=" << infantryType << ",";             // display enum
    oss << "infantryType=" << Unit::to_string_type(type) << ","; // display string
    oss << "quantity=" << quantity << ",";
    oss << "weight=" << weight << ",";
    oss << "position="; oss << pos;
    //oss << "," << "armyBelonging=" << armyBelonging;
    oss << "]";

    return oss.str();
}

//                                   .---------.
// ----------------------------------| VEHICLE |----------------------------------
//                                   '---------'
Vehicle::Vehicle(int quantity, int weight, const Position& pos, VehicleType vehicleType, int armyBelonging)
    : Unit{ quantity,weight,pos,armyBelonging }, vehicleType{ vehicleType }
{
    type = Unit::to_unit_type(vehicleType);
    initialize_attack_score_by_formula();
}
Vehicle::Vehicle(Unit* u) :
    Vehicle{ u->get_quantity(),u->get_weight(),u->getCurrentPosition(),
    Unit::to_vehicle_type(u->get_unit_type()),
    u->get_army_belonging() } {}
void     Vehicle::initialize_attack_score_by_formula()
{
    attack_score = ceil((vehicleType * 304 + quantity * weight) / double(30));
}
int      Vehicle::getAttackScore()
{
    return attack_score;
}
string   Vehicle::str() const
{
    ostringstream oss{};
    oss << Miscellanea::TAB << "Vehicle[";
    //oss << "vehicleType=" << vehicleType << ",";              // display enum
    oss << "vehicleType=" << Unit::to_string_type(type) << ","; // display string
    oss << "quantity=" << quantity << ",";
    oss << "weight=" << weight << ",";
    oss << "position="; oss << pos;
    //oss << "," << "armyBelonging=" << armyBelonging;
    oss << "]";

    return oss.str();
}

//                                   .----------.
// ----------------------------------| UNITLIST |----------------------------------
//                                   '----------'
UnitList::unit_node::unit_node(Unit* data)
    : index{ UnitList::node_index++ }, value{ data }, next{} {}
UnitList::UnitList()
    : capacity{}, size{}, vehicle_count{}, infantry_count{}, first_unit{}, last_unit{}, first_vehicle{} {}
int     UnitList::node_index{};
bool    UnitList::insert_first(Unit* u)
{
    unit_node* new_u = new unit_node{ u };
    if (!first_unit)
        first_unit = last_unit = new_u;
    else
    { 
        new_u->next = first_unit;
        first_unit = new_u;
    }
    size++;
    infantry_count++; // Only Infantry is inserted at the beginning
    return true;
}
bool    UnitList::insert_last(Unit* u)
{
    unit_node* new_u = new unit_node{ u };
    // Vehicles are always added in the end (after an old one), 
    // thus the 1st vehicle to be added will always be the first one
    if (!first_unit) first_unit = last_unit = first_vehicle = new_u;
    else
    {
        last_unit->next = new_u;
        last_unit = new_u;
        if (vehicle_count == 0) first_vehicle = new_u;
    }
    size++;
    vehicle_count++; // Only Vehicle is inserted at the end
    return true;
}
bool    UnitList::insert(Unit* u)
{
    if (size == capacity) // List is full
        return false;

    for (unit_node* i{ first_unit }; i != nullptr; i = i->next)
        if (Unit::is_equal(u, i->value))
        {
            i->value->increase_quantity_by(u->get_quantity());
            size++;
            return true;
        }

    if (dynamic_cast<Vehicle*>(u))
        return insert_last(u);  // No same Vehicle found, insert to the end
    else if (dynamic_cast<Infantry*>(u))
        return insert_first(u); // No same Infantry found, insert to the beginning
    else return false;          // something wrong
}
bool    UnitList::remove(unit_node*& u)
{
    if (!first_unit || !u) return false;

    if (first_unit == u) 
    {
        first_unit = first_unit->next;
        delete u;
        u = nullptr;
        size--;
        return true;
    }

    unit_node* current{ first_unit };
    while (current->next && (current->next != u))
        current = current->next;

    if (current->next) // Found the node to remove
    {
        current->next = u->next;
        delete u;
        u = nullptr;
        size--;
        return true;
    }

    return false;
}
bool    UnitList::remove(int n)
{
    unit_node* current{ first_unit };
    while (current && (current->index != n))
        current = current->next;

    if (current) return remove(current); // node found
    else         return false;           // node not found
}
bool    UnitList::isContain(VehicleType t) const
{
    for (unit_node* i{ first_unit }; i != nullptr; i = i->next)
        if (auto* v = dynamic_cast<Vehicle*>(i->value))
            if (Unit::to_unit_type(t) == i->value->get_unit_type())
                return true;

    return false;
}
bool    UnitList::isContain(InfantryType t) const
{
    for (unit_node* i{ first_unit }; i != nullptr; i = i->next)
        if (auto* v = dynamic_cast<Infantry*>(i->value))
            if (Unit::to_unit_type(t) == i->value->get_unit_type())
                return true;

    return false;
}
string  UnitList::str() const
{
    ostringstream oss{};
    oss << "UnitList[";
    oss << "count_vehicle=" << vehicle_count << ";";
    oss << "count_infantry=" << infantry_count << ";";
    oss << Miscellanea::NEWLINE;
    for (unit_node* i{ first_unit }; i != nullptr; i = i->next)
    {
        if (Miscellanea::DISPLAY_NODE_INDEX) oss << "<" << i->index << "> ";
        oss << i->value->str().c_str() << "," << Miscellanea::NEWLINE;
    }
    oss.seekp(-1, ios_base::end); // Remove trailing ',' from the list
    oss << "]";

    return oss.str();
}
int     UnitList::power_sum(int X, int power, int base)
{
    /*
    * At first I tried with type bool instead of int, but once N>X in one branch of the recursion tree
    * and it backtracks up to the root, the function stops searching for other branch and return false.
    * false && (_) will immediately return false without evaluating (_)
    */
    int N = pow(base, power);
    if (N > X)  return 0;
    if (N == X) return 1;
    return power_sum(X - N, power + 1, base) // Include base^power and search with next power
         + power_sum(X, power + 1, base);    // Skip base^power and search with next power
}
int     UnitList::power_sum_with_record(int X, int power, int base, vector<int>& combination, vector<vector<int>>& results) {
    int N = pow(base, power);
    if (N > X) return 0;
    if (N == X) 
    {
        combination.push_back(power);       // Include the last power term
        results.push_back(combination); // Record the valid combination
        combination.pop_back();         // Backtrack
        return 1;                       
    }

    // Include base^power and search with next power
    combination.push_back(power);
    int include = power_sum_with_record(X - N, power + 1, base, combination, results);
    combination.pop_back(); // Backtrack

    // Skip base^power and search with next power
    int exclude = power_sum_with_record(X, power + 1, base, combination, results);

    return include + exclude; 
}
void    UnitList::process_capacity(int S)
{
    bool perfect_S{ false };
    if (Miscellanea::DISPLAY_POWER_SUM) // Display all combinations that make S perfect
    {
        vector<int> combination;
        vector<vector<int>> records;
        ostringstream oss{};
        oss << "S = " << to_string(S).c_str();
        for (int prime : vector<int>{ 3,5,7 })
            if (UnitList::power_sum_with_record(S, 0, prime, combination, records))
            {
                perfect_S = true;
                oss << " = ";
                for (const auto& comb : records) 
                {
                    for (int num : comb) 
                        oss << to_string(prime).c_str() << "^" << to_string(num).c_str() << " + ";
                }
                oss.seekp(-3, ios_base::end);
                combination.clear(); records.clear();
                // break; // Only display those combinations of the 1st possible base
            }
        oss << "  " << endl;
        cout << oss.str();
    }
    else
    {
        for (int prime : vector<int>{ 3,5,7 })
            if (UnitList::power_sum(S, 0, prime))
            {
                perfect_S = true;
                break;
            }
    }
    capacity = perfect_S ? 12 : 8;
}
Unit*   UnitList::get_node_data(unit_node* u)
{
    return u->value;
}
int     UnitList::get_node_index(unit_node* u)
{
    return u->index;
}
void    UnitList::clear_vehicles() 
{
    // Move last unit to the node before first_vehicle
    unit_node* find_new_last{ first_unit };
    while (find_new_last && find_new_last->next != first_vehicle)
        find_new_last = find_new_last->next;
    last_unit = find_new_last;

    while (first_vehicle) 
    {
        unit_node* temp{ first_vehicle };
        first_vehicle = first_vehicle->next;
        delete temp;
        --size;
    }
    vehicle_count = 0;
}
void    UnitList::clear_infantries() 
{
    // first_unit will automatically become first_vehicle after the loop
    while (first_unit && first_unit != first_vehicle) 
    {
        unit_node* temp = first_unit;
        first_unit = first_unit->next;
        delete temp;
        --size;
    }
    infantry_count = 0;
}
void    UnitList::clear() 
{
    while (first_unit) 
    {
        unit_node* temp = first_unit;
        first_unit = first_unit->next;
        delete temp;
    }
    first_unit = last_unit = first_vehicle = nullptr;
    size = capacity = vehicle_count = infantry_count = 0;
};
template <typename Function> void UnitList::for_each_infantry(Function f) 
{
    for (unit_node* u{ first_unit }; u != first_vehicle; u = u->next) f(u);
}
template <typename Function> void UnitList::for_each_vehicle(Function f)
{
    for (unit_node* u{ first_vehicle }; u != nullptr; u = u->next) f(u);
}
template <typename Function> void UnitList::for_each_unit(Function f)
{
    for (unit_node* u{ first_unit }; u != nullptr; u = u->next) f(u);
}
vector<vector<int>> UnitList::to_vector(unit_node* first, unit_node* last)
{
    vector<vector<int>> v;
    for (unit_node* u{ first }; u != last; u = u->next)
        v.emplace_back(u->value->getAttackScore(), u->index);
    return v;
}
void UnitList::find_subsets(vector<vector<int>>& scores_indices, int target, vector<vector<int>>& subset,
                            vector<vector<vector<int>>>& best_subsets,
                            int index, int& min_sum, int current_sum)
{
    if (current_sum >= target)              // Current subset is valid ...
    {
        if (current_sum < min_sum)          // and has smaller sum than the current minimum one
        {
            min_sum = current_sum;
            best_subsets.clear();
        }
        if (current_sum == min_sum)          // and has same sum as the current minimum one
            best_subsets.push_back(subset);

        return;
    }

    if (index >= scores_indices.size())      // Reach end of array
        return;

    subset.push_back(scores_indices[index]); // Include the current element
    find_subsets(scores_indices, target, subset, best_subsets, index + 1, min_sum, current_sum + scores_indices[index][0]);

    subset.pop_back();                       // Exclude the current element
    find_subsets(scores_indices, target, subset, best_subsets, index + 1, min_sum, current_sum);
}
vector<vector<vector<int>>> UnitList::minimum_subsets(vector<vector<int>>& nums, int target)
{
    vector<vector<int>> subset;
    vector<vector<vector<int>>> best_subsets;
    int minimum = INT_MAX;
    find_subsets(nums, target, subset, best_subsets, 0, minimum, 0);

    if (Miscellanea::PRINT_MINIMAL_SUBSETs)
    {
        cout << "target = " << target << " -> minimum sum = " << minimum <<" : <index>score" << endl;
        for (vector<vector<int>> subset : best_subsets)
        {
            cout << "   =   ";
            for (vector<int>element : subset)
                cout << "<" << element[1] << ">" << element[0] << " + ";
            cout << endl;
        }
    }

    return best_subsets;
}
UnitList::~UnitList()
{
    unit_node* u{ first_unit };
    while (u)
    {
        unit_node* t{ u };
        u = u->next;
        delete t;
    }
}

//                                   .------.
// ----------------------------------| ARMY |----------------------------------
//                                   '------'
Army::Army(Unit** unitArray, int size, string name, BattleField* battleField)
    : name{ name }, battleField{ battleField }, unitList{ new UnitList{} }, LF{}, EXP{}, liberation{}
{
    for (int i{}; i < size; ++i)
    {
        if (auto* v = dynamic_cast<Infantry*>(unitArray[i]))
            EXP += v->getAttackScore();
        else if (auto * v = dynamic_cast<Vehicle*>(unitArray[i]))
            LF += v->getAttackScore();
    }

    // Calculate UnitList::capacity
    int S{ LF + EXP };
    unitList->process_capacity(S);

    // Insert units
    for (int i{}; i < unitList->capacity && i < size; ++i) unitList->insert(unitArray[i]);

    //  Re-calculate LF & EXP
    calculate_EXP(); calculate_LF();
}
void Army::calculate_EXP()
{
    int total_EXP{};
    unitList->for_each_infantry([&](UnitList::unit_node* u) {
        total_EXP += UnitList::get_node_data(u)->getAttackScore(); });
    EXP = total_EXP < 1000 ? total_EXP : 1000;
}
void Army::calculate_LF()
{
    int total_LF{};
    unitList->for_each_vehicle([&](UnitList::unit_node* u) {
        total_LF += UnitList::get_node_data(u)->getAttackScore(); });
    LF = total_LF < 500 ? total_LF : 500;
}
void Army::adjust_EXP(int n) { EXP += n; }
void Army::adjust_LF(int n) { LF += n; }
int  Army::get_EXP() { return EXP; }
int  Army::get_LF() { return LF; }
bool Army::is_liberation() const { return liberation; }
Army::~Army()
{
    delete unitList;
}

//                                   .-----------------.
// ----------------------------------| LIBERATION ARMY |----------------------------------
//                                   '-----------------'
LiberationArmy::LiberationArmy(Unit** unitArray, int size, string name, BattleField* battleField)
    :Army{ unitArray,size,name,battleField } 
{
    liberation = true;
}
int     LiberationArmy::next_fibonacci(int n)
{
    if (n <= 0) return 0;
    if (n == 1) return 1;

    int a{}, b{ 1 };
    while (b < n) 
    {
        int temp = b;
        b = a + b;
        a = temp;
    }
    return b;
}
bool    LiberationArmy::fight(Army* enemy, bool defense)
{
    // There may be more than 1 minimum subsets found, but for now, remove those nodes belonging to the 1st one
    
    bool victory{ false }; // battle occurs => victory

    if (defense)
    {
        LF = ceil(LF * 1.3); EXP = ceil(EXP * 1.3);
        if      (LF >= enemy->get_LF() && EXP >= enemy->get_EXP()); // Victory
        else if (LF < enemy->get_LF() && EXP < enemy->get_EXP())    // Lose -> next fibonacci -> re-fight
        {
            unitList->for_each_unit([this](UnitList::unit_node* u) {   // Each unit's quantity become its next fibonacci
                UnitList::get_node_data(u)->set_quantity(LiberationArmy::next_fibonacci(UnitList::get_node_data(u)->get_quantity())); });            
            calculate_EXP(); calculate_LF();                        // Update battle index
            LiberationArmy::fight(enemy, defense);                  // re-fight
        }
        else
        {
            unitList->for_each_unit([this](UnitList::unit_node* u) {   // Each unit loses 10% quantity
                UnitList::get_node_data(u)->set_quantity(ceil(0.9 * UnitList::get_node_data(u)->get_quantity())); });
            calculate_EXP(); calculate_LF();                        // Update battle index
        }
    }
    else
    {
        LF = ceil(LF * 1.5); EXP = ceil(EXP * 1.5);
        vector<vector<int>> infantry_scores_indices{ UnitList::to_vector(unitList->first_unit,unitList->first_vehicle) };
        vector<vector<vector<int>>> combination_A{ unitList->minimum_subsets(infantry_scores_indices,enemy->get_EXP()) };
        vector<vector<int>> vehicle_scores_indices{ UnitList::to_vector(unitList->first_vehicle,unitList->last_unit) };
        vector<vector<vector<int>>> combination_B{ unitList->minimum_subsets(vehicle_scores_indices,enemy->get_LF()) };
        
        if (!combination_A.empty() && !combination_B.empty())   // --- Both combinations found
        {
            victory = true;
            // Remove units of minimum subsets
            for (vector<int>score_index : combination_A[0])  unitList->remove(score_index[1]);
            for (vector<int>score_index : combination_B[0])  unitList->remove(score_index[1]);
            // Update battle index
            calculate_EXP(); calculate_LF(); 
        }
        else if (!combination_A.empty())    // ----------------------- Only combination A found
        {
            if (LF > enemy->get_LF())
            {
                victory = true;
                
                for (vector<int>score_index : combination_A[0]) // Remove only those infantries of minimum subsets
                    unitList->remove(score_index[1]);
                unitList->clear_vehicles();                     // Remove all vehicles
                calculate_EXP(); calculate_LF();                // Update battle index
            }
        }
        else if (!combination_B.empty())    // ----------------------- Only combination B found
        {
            if (EXP > enemy->get_EXP())
            {
                victory = true;
                
                for (vector<int>score_index : combination_B[0]) // Remove only those vehicles of minimum subsets
                    unitList->remove(score_index[1]);
                unitList->clear_infantries();                   // Remove all infantries
                calculate_EXP(); calculate_LF();                // Update battle index
            }
        }
        else; // ----------------------------------------------------- No combination found

        if (victory) // Battle occured
        {
            vector<UnitList::unit_node*> confiscated_units;
            enemy->unitList->for_each_unit([this, &confiscated_units](UnitList::unit_node* u) {
                if (unitList->insert(UnitList::get_node_data(u)))   // if steal (copy) each enemy's unit successfully
                confiscated_units.push_back(u);                     // save for later removal
                });  
            calculate_EXP(); calculate_LF();                        // Update battle index

            for (UnitList::unit_node* u : confiscated_units)        // remove the confiscated units
                enemy->unitList->remove(u); 
        }
        else         // No battle
        {
            unitList->for_each_unit([this](UnitList::unit_node* u) {   // Each unit loses 10% weight
                UnitList::get_node_data(u)->set_weight(ceil(0.9 * UnitList::get_node_data(u)->get_weight())); });
            calculate_EXP(); calculate_LF();                        // Update battle index
        }
    }

    return victory;
}
string  LiberationArmy::str() const
{
    ostringstream oss{};
    oss << "LiberationArmy[";
    oss << "LF=" << LF << ",";
    oss << "EXP=" << EXP << ",";
    oss << "unitList=" << unitList->str().c_str();
    //oss << "," << "battleField=" << battleField;
    oss << "]";

    return oss.str();
}

//                                   .------.
// ----------------------------------| ARVN |----------------------------------
//                                   '------'
ARVN::ARVN(Unit** unitArray, int size, string name, BattleField* battleField)
    :Army{ unitArray,size,name,battleField } 
{
    liberation = false;
}
bool    ARVN::fight(Army* enemy, bool defense)
{
    if (defense)
    {
        // LA fight with defense = false??
        if (enemy->fight(this, false)) // if LA fights this and win (also removes some/all of this's unit)
        {
            unitList->for_each_unit([this](UnitList::unit_node* u) {   // Each remaining unit's quantity reduce by 20% weight
                UnitList::get_node_data(u)->set_weight(ceil(UnitList::get_node_data(u)->get_weight() * 0.8)); });
        }
    }
    else
    {
        unitList->for_each_unit([this](UnitList::unit_node* u) {   // Each unit's quantity reduces to 80%
            int reduced_quantity = ceil(0.8 * (UnitList::get_node_data(u)->get_quantity()));
        if (reduced_quantity == 1) // remove when quantity = 1
            unitList->remove(u);
        else                       // update quantity
            UnitList::get_node_data(u)->set_quantity(reduced_quantity);
            });
        // Update battle index
        calculate_EXP(); calculate_LF();
    }

    return true; // for no reason ?
}
string  ARVN::str() const
{
    ostringstream oss{};
    oss << "ARVN[";
    oss << "LF=" << LF << ",";
    oss << "EXP=" << EXP << ",";
    oss << "unitList=" << unitList->str().c_str();
    //oss << "," << "battleField=" << battleField;
    oss << "]";

    return oss.str();
}

//                                   .-------------.
// ----------------------------------| BATTLEFIELD |----------------------------------
//                                   '-------------'
BattleField::BattleField(int n_rows, int n_cols, vector<Position*> arrayForest,
    vector<Position*> arrayRiver, vector<Position*> arrayFortification,
    vector<Position*> arrayUrban, vector<Position*> arraySpecialZone) 
    : n_rows{ n_rows }, n_cols{ n_cols }
{
    vector<TerrainElement*> forests, rivers, fortifications, urbans, specials;
    for (Position* p : arrayForest)        forests.push_back(new Mountain{ *p });
    for (Position* p : arrayRiver)         rivers.push_back(new Mountain{ *p });
    for (Position* p : arrayFortification) fortifications.push_back(new Mountain{ *p });
    for (Position* p : arrayUrban)         urbans.push_back(new Mountain{ *p });
    for (Position* p : arraySpecialZone)   specials.push_back(new Mountain{ *p });

    terrain.push_back(forests);
    terrain.push_back(rivers);
    terrain.push_back(fortifications);
    terrain.push_back(urbans);
    terrain.push_back(specials);
}
BattleField::~BattleField()
{
    for (vector<TerrainElement*>& v : terrain)
        for (auto* p : v) delete p;
}
//                                   .-----------------.
// ----------------------------------| TERRAIN ELEMENT |----------------------------------
//                                   '-----------------'
TerrainElement::TerrainElement(Position pos) : pos{ pos } {}
TerrainElement::~TerrainElement() {}
int TerrainElement::euclid_distance(const Position& a, const Position& b)
{
    return ceil(sqrt(pow(a.getRow() - b.getRow(), 2) + pow(a.getCol() - b.getCol(), 2)));
}
void TerrainElement::getEffect(Army* army) 
{
    army->unitList->for_each_infantry([&](UnitList::unit_node* u) {
        affect_infantry(army, u); });
    army->unitList->for_each_vehicle([&](UnitList::unit_node* u) {
        affect_vehicle(army, u); });
}
//                                   .------.
// ----------------------------------| ROAD |----------------------------------
//                                   '------'
void Road::affect_infantry(Army* a, UnitList::unit_node* u) {}
void Road::affect_vehicle(Army* a, UnitList::unit_node* u) {}

//                                   .----------.
// ----------------------------------| MOUNTAIN |----------------------------------
//                                   '----------'
void Mountain::affect_infantry(Army* a, UnitList::unit_node* u)
{
    Unit* node{ UnitList::get_node_data(u) };
    int distance{ TerrainElement::euclid_distance(pos,node->getCurrentPosition()) };
    if (a->is_liberation())
    {
        if (distance <= 2)
            a->adjust_EXP(ceil(node->getAttackScore() * 0.3));
    }
    else
    {
        if (distance <= 4)
            a->adjust_EXP(ceil(node->getAttackScore() * 0.2));
    }
}
void Mountain::affect_vehicle(Army* a, UnitList::unit_node* u)
{
    Unit* node{ UnitList::get_node_data(u) };
    int distance{ TerrainElement::euclid_distance(pos,node->getCurrentPosition()) };
    if (a->is_liberation())
    {
        if (distance <= 2)
            a->adjust_EXP(ceil(node->getAttackScore() * -0.1));
    }
    else
    {
        if (distance <= 4)
            a->adjust_EXP(ceil(node->getAttackScore() * -0.05));
    }
}

//                                   .-------.
// ----------------------------------| RIVER |----------------------------------
//                                   '-------'
void River::affect_infantry(Army* a, UnitList::unit_node* u)
{
    Unit* node{ UnitList::get_node_data(u) };
    int distance{ TerrainElement::euclid_distance(pos,node->getCurrentPosition()) };
    if (distance <= 2)
        node->setAttackScore(ceil(0.8 * node->getAttackScore()));
}
void River::affect_vehicle(Army* a, UnitList::unit_node* u) {}

//                                   .-------.
// ----------------------------------| URBAN |----------------------------------
//                                   '-------'
void Urban::affect_infantry(Army* a, UnitList::unit_node* u)
{
    Unit* node{ UnitList::get_node_data(u) };
    UnitType type{ node->get_unit_type() };
    int distance{ TerrainElement::euclid_distance(pos,node->getCurrentPosition()) };
    if (a->is_liberation())
    {
        if (distance <= 5 && (type == UnitType::SPECIALFORCES || type == UnitType::REGULARINFANTRY))
            node->setAttackScore(ceil((double)2 * node->getAttackScore() / distance));
    }
    else
    {
        if (distance <= 3 && type == UnitType::REGULARINFANTRY)
            node->setAttackScore(ceil((double)(3 * node->getAttackScore()) / (2 * distance)));
    }
}
void Urban::affect_vehicle(Army* a, UnitList::unit_node* u)
{
    Unit* node{ UnitList::get_node_data(u) };
    UnitType type{ node->get_unit_type() };
    int distance{ TerrainElement::euclid_distance(pos,node->getCurrentPosition()) };
    if (a->is_liberation())
    {
        if (distance <= 2 && type == UnitType::ARTILLERY)
            node->setAttackScore(ceil(0.5 * node->getAttackScore()));
    }
}

//                                   .---------------.
// ----------------------------------| FORTIFICATION |----------------------------------
//                                   '---------------'
void Fortification::affect_infantry(Army* a, UnitList::unit_node* u)
{
    Unit* node{ UnitList::get_node_data(u) };
    int distance{ TerrainElement::euclid_distance(pos,node->getCurrentPosition()) };
    if (a->is_liberation())
    {
        if (distance <= 2)
            node->setAttackScore(ceil(0.8 * node->getAttackScore()));
    }
    else
    {
        if (distance <= 2)
            node->setAttackScore(ceil(1.2 * node->getAttackScore()));
    }
}
void Fortification::affect_vehicle(Army* a, UnitList::unit_node* u)
{
    Unit* node{ UnitList::get_node_data(u) };
    int distance{ TerrainElement::euclid_distance(pos,node->getCurrentPosition()) };
    if (a->is_liberation())
    {
        if (distance <= 2)
            node->setAttackScore(ceil(0.8 * node->getAttackScore()));
    }
    else
    {
        if (distance <= 2)
            node->setAttackScore(ceil(1.2 * node->getAttackScore()));
    }
}

//                                   .--------------.
// ----------------------------------| SPECIAL ZONE |----------------------------------
//                                   '--------------'
void SpecialZone::affect_infantry(Army* a, UnitList::unit_node* u)
{
    Unit* node{ UnitList::get_node_data(u) };
    int distance{ TerrainElement::euclid_distance(pos,node->getCurrentPosition()) };
    if (distance <= 1) node->setAttackScore(0);
}
void SpecialZone::affect_vehicle(Army* a, UnitList::unit_node* u)
{
    Unit* node{ UnitList::get_node_data(u) };
    int distance{ TerrainElement::euclid_distance(pos,node->getCurrentPosition()) };
    if (distance <= 1) node->setAttackScore(0);
}

//                                   .-------------.
// ----------------------------------| HCMCAMPAIGN |----------------------------------
//                                   '-------------'
HCMCampaign::HCMCampaign(const string& file_path)
{
    config = new Configuration{ file_path };
    battleField = new BattleField{ config->num_cols,config->num_rows,
        config->arrayForest,
        config->arrayRiver,
        config->arrayFortification,
        config->arrayUrban,
        config->arraySpecialZone };
    liberationArmy = new LiberationArmy{ config->liberationUnits,
        config->liberation_unit_count,Miscellanea::LIBERATION_ARMY_NAME,battleField };
    ARVNArmy = new ARVN{ config->ARVNUnits,
        config->arvn_unit_count,Miscellanea::ARVN_NAME,battleField };
}
HCMCampaign::~HCMCampaign()
{
    delete config;
    delete battleField;
    delete liberationArmy;
    delete ARVNArmy;
};
void   HCMCampaign::run()
{
    if (Miscellanea::PRINT_CONFIG_WHEN_HCMC_RUN) 
    {
        cout << config->str();
        cout << "\n------------------------------------------------------------------\n\n";
    }

    for (vector<TerrainElement*>& v : battleField->terrain)
        for (auto* p : v) 
        {
            p->getEffect(liberationArmy);
            p->getEffect(ARVNArmy);
        }

    if (config->eventCode < 75) liberationArmy->fight(ARVNArmy, false);
    else
    {
        ARVNArmy->fight(liberationArmy, false);
        liberationArmy->fight(ARVNArmy, false);
    }
}
string HCMCampaign::printResult()
{
    ostringstream oss{};
    oss << "LIBERATIONARMY[LF=" << liberationArmy->get_LF() << ",EXP =" << liberationArmy->get_EXP();
    oss << "]-ARVN[LF=" << ARVNArmy->get_LF() << ",EXP=" << ARVNArmy->get_EXP();
    oss << "]";
    return oss.str();
}