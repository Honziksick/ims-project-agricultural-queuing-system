#ifndef FARM_CLASSES_H
#define FARM_CLASSES_H

#include "simlib.h"
#include "config.h" // Potřebujeme config pro typy
#include <string>
#include <vector>
#include <iostream>
#include <map>

enum FieldPhase {
    PHASE_START = 0,
    PHASE_MIN_TILL,      
    PHASE_STUBBLE,       
    PHASE_PLOW,          
    PHASE_PREP,          
    PHASE_FERTILIZE,     
    PHASE_SOWING,        
    PHASE_ROLLING,
    PHASE_DONE
};

const std::string PhaseNames[] = {
    "Start",
    "Min-Till",
    "Stubble",
    "Plow",
    "Prep",
    "Fertilize",
    "Sowing",
    "Rolling",
    "Done"
};

struct Field {
    int id;
    FieldPhase currentPhase;
    bool isMinTillPath;     
    bool isBeingWorkedOn;   
    double workDoneInPhase; 
    
    double accumulatedYieldCZK; 
    double accumulatedCostCZK;  

    Field(int _id) : id(_id), currentPhase(PHASE_START), 
                     isMinTillPath(false), isBeingWorkedOn(false), 
                     workDoneInPhase(0), 
                     accumulatedYieldCZK(0.0), accumulatedCostCZK(0.0) {}
};

// --- Globals ---
extern bool IsDayWorkable;

// Pointers to Stores (Dynamically allocated based on Config)
extern Store* pTractors;
extern Store* pMachineMinTill;
extern Store* pMachineStubble;
extern Store* pMachinePlow;
extern Store* pMachinePrep;
extern Store* pMachineFertilizer;
extern Store* pMachineSower;
extern Store* pMachineRoller;

// Lookup Arrays
extern Store* MachineRequirements[9]; 
extern double PhaseDurations[9];

extern std::vector<Field> FarmFields;
extern std::map<int, double> FieldActiveDurations; 

// Functions
void AddMaterialCost(Field* field, double costPerHa);
void LogEvent(std::string actor, std::string action);
std::string GetFormattedTime(double t);

#endif