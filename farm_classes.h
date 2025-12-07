#ifndef FARM_CLASSES_H
#define FARM_CLASSES_H

#include "simlib.h"
#include "config.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

// ============================================================================
//  ENUMS & CONSTANTS
// ============================================================================

enum FieldPhase {
    PHASE_START = 0,
    PHASE_MIN_TILL,      
    PHASE_STUBBLE,       
    PHASE_PLOW,          
    PHASE_PREP,          
    PHASE_FERTILIZE,     
    PHASE_SOWING,        
    PHASE_ROLLING,
    PHASE_DONE,
    NUM_PHASES // Helper for array sizing
};

const std::string PhaseNames[] = {
    "Start", "Min-Till", "Stubble", "Plow", "Prep", 
    "Fertilize", "Sowing", "Rolling", "Done"
};

// ============================================================================
//  DOMAIN ENTITIES
// ============================================================================

struct Field {
    int id;
    FieldPhase currentPhase;
    bool isMinTillPath;     
    bool isBeingWorkedOn;   
    double workDoneInPhase; 
    
    // Economic tracking per field
    double accumulatedYieldCZK; 
    double accumulatedCostCZK;  

    Field(int _id) : 
        id(_id), 
        currentPhase(PHASE_START), 
        isMinTillPath(false), 
        isBeingWorkedOn(false), 
        workDoneInPhase(0), 
        accumulatedYieldCZK(0.0), 
        accumulatedCostCZK(0.0) {}
};

struct SimStats {
    // Financials
    double totalProfit;
    double totalRevenue;
    double maxPotentialRevenue;
    double totalLoss;
    
    // Expenses
    double costLabor;
    double costMachine;
    double costMaterial;
    double totalExpenses;

    // Operational Metrics
    double simEndTime;
    double daysWaitingForWindow; 
    int unfinishedFieldsCount;
    int workableDays;
    bool allFieldsFinished;

    // Utilization (%)
    double avgTractorUtil;
    double avgSowerUtil;
};

// ============================================================================
//  SIMULATION CONTEXT
// ============================================================================

/**
 * Holds the entire dynamic state of one simulation run.
 * Replaces loose global variables.
 */
struct SimulationContext {
    // Config for this run
    SimConfig cfg;

    // State Variables
    bool isDayWorkable;
    double timeAllReadyForSowing;
    int globalWorkableDaysCount;
    
    // Cost Accumulators
    double totalWorkerWages;
    double totalMachineCost;
    double totalMaterialCost;

    // Utilization Accumulators
    double totalTractorHours;
    double totalSowerHours;

    // Resources (SIMLIB Stores)
    Store* pTractors;
    Store* pMachineMinTill;
    Store* pMachineStubble;
    Store* pMachinePlow;
    Store* pMachinePrep;
    Store* pMachineFertilizer;
    Store* pMachineSower;
    Store* pMachineRoller;

    // Logic Lookups
    Store* machineRequirements[NUM_PHASES];
    double phaseDurations[NUM_PHASES];

    // Data containers
    std::vector<Field> farmFields;
    std::map<int, double> fieldActiveDurations; 

    // Constructor initializes basics, pointers null until Setup()
    SimulationContext() : 
        isDayWorkable(true), 
        timeAllReadyForSowing(-1.0),
        globalWorkableDaysCount(0),
        totalWorkerWages(0), totalMachineCost(0), totalMaterialCost(0),
        totalTractorHours(0), totalSowerHours(0),
        pTractors(nullptr), pMachineMinTill(nullptr), pMachineStubble(nullptr),
        pMachinePlow(nullptr), pMachinePrep(nullptr), pMachineFertilizer(nullptr),
        pMachineSower(nullptr), pMachineRoller(nullptr) 
    {}
};

#endif // FARM_CLASSES_H