// ============================================================================
// Parking Lot Low-Level Design (LLD)
// ============================================================================

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

// ---------------------------------------------------------
// ENUMS
// ---------------------------------------------------------
enum class VehicleType { MOTORCYCLE, CAR, TRUCK };
enum class SpotType { MOTORCYCLE, COMPACT, LARGE };

// ---------------------------------------------------------
// CORE ENTITIES
// ---------------------------------------------------------

/**
 * @class Vehicle
 * @brief Represents a customer's vehicle containing its unique license plate
 * and type.
 */
class Vehicle {
  string licensePlate;
  VehicleType type;

public:
  Vehicle(string plate, VehicleType t) : licensePlate(plate), type(t) {}

  string getLicensePlate() const { return licensePlate; }
  VehicleType getType() const { return type; }
};

/**
 * @class VehicleFactory
 * @brief Factory class to encapsulate the creation logic of Vehicles.
 */
class VehicleFactory {
public:
  static shared_ptr<Vehicle> createVehicle(const string &licensePlate,
                                           VehicleType type) {
    if (licensePlate.empty()) {
      throw invalid_argument("License plate cannot be empty.");
    }
    return make_shared<Vehicle>(licensePlate, type);
  }
};

/**
 * @class ParkingSpot
 * @brief Represents a single parking space which can hold one vehicle at a
 * time.
 */
class ParkingSpot {
  string spotId;
  SpotType spotType;
  bool available;
  shared_ptr<Vehicle> parkedVehicle;

public:
  ParkingSpot(string id, SpotType t)
      : spotId(id), spotType(t), available(true), parkedVehicle(nullptr) {}

  string getSpotId() const { return spotId; }
  SpotType getSpotType() const { return spotType; }
  bool isAvailable() const { return available; }

  void parkVehicle(shared_ptr<Vehicle> &v) {
    parkedVehicle = v;
    available = false;
  }

  void removeVehicle() {
    parkedVehicle = nullptr;
    available = true;
  }
};

/**
 * @class ParkingConfiguration
 * @brief Manages the mappings between Vehicle Types and corresponding Spot
 * Types.
 */
class ParkingConfiguration {
  unordered_map<VehicleType, SpotType> vehicleTypeToSpotTypeMap;

public:
  ParkingConfiguration() {
    // Default Mapping Initialisation
    vehicleTypeToSpotTypeMap[VehicleType::CAR] = SpotType::COMPACT;
    vehicleTypeToSpotTypeMap[VehicleType::MOTORCYCLE] = SpotType::MOTORCYCLE;
    vehicleTypeToSpotTypeMap[VehicleType::TRUCK] = SpotType::LARGE;
  }

  SpotType getRequiredSpotType(VehicleType vtype) const {
    auto it = vehicleTypeToSpotTypeMap.find(vtype);
    if (it != vehicleTypeToSpotTypeMap.end()) {
      return it->second;
    } else {
      throw invalid_argument("Vehicle Type is not supported by the current "
                             "Parking Configuration.");
    }
  }

  void registerMapping(VehicleType v, SpotType s) {
    vehicleTypeToSpotTypeMap[v] = s;
  }
};

/**
 * @class ParkingFloor
 * @brief Represents a logical grouping/floor of parking spots in the system.
 */
class ParkingFloor {
  string floorId;
  vector<shared_ptr<ParkingSpot>> spots;

public:
  ParkingFloor(string id) : floorId(id) {}

  void addParkingSpot(shared_ptr<ParkingSpot> &spot) { spots.push_back(spot); }

  shared_ptr<ParkingSpot> findAvailableSpot(SpotType requiredType) {
    for (shared_ptr<ParkingSpot> &spot : spots) {
      if (spot->isAvailable() && spot->getSpotType() == requiredType) {
        return spot;
      }
    }
    return nullptr; // No spot found matching the required criteria
  }
};

/**
 * @class Ticket
 * @brief The receipt generated upon successful entry, associating a vehicle to
 * its spot.
 */
class Ticket {
  string ticketId;
  shared_ptr<Vehicle> vehicle;
  shared_ptr<ParkingSpot> spot;

public:
  Ticket(string id, shared_ptr<Vehicle> &v, shared_ptr<ParkingSpot> &s)
      : ticketId(id), vehicle(v), spot(s) {}

  string getTicketId() const { return ticketId; }
  shared_ptr<Vehicle> getVehicle() const { return vehicle; }
  shared_ptr<ParkingSpot> getSpot() const { return spot; }
};

// ---------------------------------------------------------
// PRICING STRATEGY (STRATEGY PATTERN)
// ---------------------------------------------------------

/**
 * @class PricingStrategy
 * @brief Abstract Base class providing an interface for calculating parking
 * bounds.
 */
class PricingStrategy {
public:
  virtual double calculateCost(VehicleType type, double hours) const = 0;
  virtual ~PricingStrategy() = default; // Virtual destructor for memory safety
};

/**
 * @class StandardPricingStrategy
 * @brief Concrete implementation providing standard flat hourly rates.
 */
class StandardPricingStrategy : public PricingStrategy {
  unordered_map<VehicleType, double> hourlyRates;

public:
  StandardPricingStrategy() {
    hourlyRates[VehicleType::MOTORCYCLE] = 10.0;
    hourlyRates[VehicleType::CAR] = 20.0;
    hourlyRates[VehicleType::TRUCK] = 40.0;
  }

  double calculateCost(VehicleType type, double hours) const override {
    auto it = hourlyRates.find(type);
    if (it != hourlyRates.end()) {
      return hours * it->second; // Calculate based on type
    }
    return hours * 20.0; // Fallback default rate
  }
};

// ---------------------------------------------------------
// ORCHESTRATOR / MANAGER CLASS (SINGLETON)
// ---------------------------------------------------------

/**
 * @class ParkingLot
 * @brief Grand orchestrator managing entry, exit, configurations and pricing
 * schemas. Defines a Singleton instance to prevent multiple conflicting states.
 */
class ParkingLot {
  string name;
  vector<shared_ptr<ParkingFloor>> floors;
  int ticketCounter;

  // Composition: strong tied relationship where ParkingLot controls lifespan
  unique_ptr<ParkingConfiguration> cfg;
  unique_ptr<PricingStrategy> pricingStrategy;

  // Private constructor prevents external instantiation (Singleton)
  ParkingLot()
      : ticketCounter(0), cfg(make_unique<ParkingConfiguration>()) {}

public:
  // Delete copy-constructor and copy assignment operators to enforce pure
  // Singleton
  ParkingLot(const ParkingLot &) = delete;
  ParkingLot &operator=(const ParkingLot &) = delete;

  // Global Access Point
  static ParkingLot &getInstance() {
    static ParkingLot instance;
    return instance;
  }

  // Inject a strategy contextually overriding earlier one
  void setPriceStrategy(unique_ptr<PricingStrategy> &ps) {
    pricingStrategy = std::move(ps); // Move ownership
  }

  void setName(string n){
    name = n;
  }

  string getName() const {
    return name;
  }

  void addFloor(shared_ptr<ParkingFloor> &floor) { floors.push_back(floor); }

  /**
   * @brief Manages logic when a vehicle arrives at the entry boom barriers.
   */
  shared_ptr<Ticket> entryPanel(shared_ptr<Vehicle> &vehicle) {
    try {
      SpotType requiredSpotType = cfg->getRequiredSpotType(vehicle->getType());

      for (shared_ptr<ParkingFloor> &floor : floors) {
        shared_ptr<ParkingSpot> spot =
            floor->findAvailableSpot(requiredSpotType);

        if (spot != nullptr) {
          spot->parkVehicle(vehicle);
          ticketCounter++;

          string tId = "TCK-" + to_string(ticketCounter);
          shared_ptr<Ticket> ticket = make_shared<Ticket>(tId, vehicle, spot);

          cout << "SUCCESS: Vehicle " << vehicle->getLicensePlate()
               << " parked at Spot " << spot->getSpotId() << endl;

          return ticket;
        }
      }
      cout << "FAILED: Parking full for vehicle " << vehicle->getLicensePlate()
           << endl;
      return nullptr;

    } catch (const invalid_argument &e) {
      cout << "ERROR: " << e.what() << endl;
      return nullptr;
    }
  }

  /**
   * @brief Manages the logic when a vehicle checks-out through exit panels.
   */
  void exitPanel(shared_ptr<Ticket> &ticket) {
    if (!ticket)
      return;

    shared_ptr<ParkingSpot> spot = ticket->getSpot();
    shared_ptr<Vehicle> vehicle = ticket->getVehicle();

    spot->removeVehicle();

    // Hardcoding a 3.0 hour stay specifically for this emulation
    double cost = pricingStrategy->calculateCost(vehicle->getType(), 3.0);

    cout << "EXIT: Vehicle " << vehicle->getLicensePlate() << " left Spot "
         << spot->getSpotId() << ". Total Charged: $" << cost << endl;
  }
};

// ---------------------------------------------------------
// MAIN DRIVER
// ---------------------------------------------------------
int main() {
  // 0. Initialise strategy
  unique_ptr<PricingStrategy> pricingStrategy =
      make_unique<StandardPricingStrategy>();

  // 1. Initialize the Parking Lot
  ParkingLot &myLot = ParkingLot::getInstance();
  myLot.setPriceStrategy(pricingStrategy);
  myLot.setName("Parking-Lot-1");

  // 2. Setup Infrastructure (1 Floor, 2 Spots: 1 Compact, 1 Motorcycle)
  auto floor1 = make_shared<ParkingFloor>("Floor-1");
  auto compactSpot = make_shared<ParkingSpot>("C-101", SpotType::COMPACT);
  auto motoSpot = make_shared<ParkingSpot>("M-201", SpotType::MOTORCYCLE);
  auto truckSpot = make_shared<ParkingSpot>("T-200", SpotType::LARGE);

  floor1->addParkingSpot(compactSpot);
  floor1->addParkingSpot(motoSpot);
  floor1->addParkingSpot(truckSpot);
  myLot.addFloor(floor1);

  // 3. Create Vehicles Using Factory Pattern
  auto car1 = VehicleFactory::createVehicle("MH-12-CAR-111", VehicleType::CAR);
  auto car2 = VehicleFactory::createVehicle("MH-12-CAR-222", VehicleType::CAR);
  auto bike1 =
      VehicleFactory::createVehicle("MH-12-BIKE-333", VehicleType::MOTORCYCLE);
  auto truck1 = VehicleFactory::createVehicle("MH-12-TRUCK-123", VehicleType::TRUCK);
  auto truck2 = VehicleFactory::createVehicle("MH-12-TRUCK-123", VehicleType::TRUCK);

  // 4. Test Entry Flow
  cout << "\n--- PROCESSING ENTRIES ---" << endl;
  auto ticket1 = myLot.entryPanel(car1);  // Should Succeed (Takes C-101)
  auto ticket2 = myLot.entryPanel(car2);  // Should Fail (No compact spots left)
  auto ticket3 = myLot.entryPanel(bike1); // Should Succeed (Takes M-201)
  auto ticket4 = myLot.entryPanel(truck1); // Should Succeed (Takes T-200)
  auto ticket5 = myLot.entryPanel(truck2); // Should Fail (No large spots left)


  // 5. Test Exit Flow (Billing via Strategy Pattern)
  cout << "\n--- PROCESSING EXITS ---" << endl;
  myLot.exitPanel(ticket1); // Computes bill using Car rate ($20 * 3 hrs = $60)
  myLot.exitPanel(
      ticket3); // Computes bill using Motorcycle rate ($10 * 3 hrs = $30)
  myLot.exitPanel(ticket4); // Computers bill using Truck rate ($40 * 3 hrs = $120)

  myLot.setName("Parking-Lot-2");
  cout<<"Parking lot name changed to: "<<myLot.getName()<<endl;

  return 0;
}