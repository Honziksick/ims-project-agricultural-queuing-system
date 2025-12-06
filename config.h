#ifndef CONFIG_H
#define CONFIG_H

#include "simlib.h"

// --- Default Constants (used to initialize config) ---
namespace Defaults {
    const double HOUR = 1.0;
    const double DAY = 24.0 * HOUR;
    const double SIMULATION_DURATION = 60.0 * DAY;

    const double PROB_WORKABLE_DAY = 0.40;
    const double SOWING_START_TIME = 19.0 * DAY; 
    const double LATE_SOWING_TIME = (19.0 + 21.0) * DAY;

    const double FIELD_SIZE_HA = 15.0;
    const double YIELD_TONNES_PER_HA = 6.5;
    const double PRICE_CZK_PER_TONNE = 4800.0;
    const double YIELD_PENALTY_PER_DAY = 0.01;

    const double COST_WORKER_PER_HOUR = 250.0;
    const double COST_TRACTOR_PER_HOUR = 900.0;
    const double COST_SEED_PER_HA = 2200.0;
    const double COST_FERTILIZER_PER_HA = 3500.0;

    const int COUNT_WORKERS_SHIFT_1 = 2;
    const int COUNT_WORKERS_SHIFT_2 = 1;
    const int COUNT_TRACTORS = 2;
    const int COUNT_FIELDS = 4;

    // Machine Counts
    const int COUNT_MACHINE_MIN_TILL = 0; 
    const int COUNT_MACHINE_STUBBLE = 1;
    const int COUNT_MACHINE_PLOW = 1;
    const int COUNT_MACHINE_PREP = 1;
    const int COUNT_MACHINE_FERTILIZER = 1;
    const int COUNT_MACHINE_SOWER = 1;
    const int COUNT_MACHINE_ROLLER = 1;

    const double SHIFT_DURATION = 10.0 * HOUR;
    const double REST_DURATION = 14.0 * HOUR;
    const double SHIFT_2_START_OFFSET = 0.0; 

    const double PROB_TRACTOR_UNAVAILABLE = 0.30;
    const double PROB_REPAIR_NEEDED = 0.28;
    const double REPAIR_TIME = 4.0 * HOUR;

    const double TIME_MIN_TILL = 5.0 * HOUR;
    const double TIME_STUBBLE = 9.5 * HOUR;
    const double TIME_PLOW = 15.5 * HOUR;
    const double TIME_PREP = 8.0 * HOUR;
    const double TIME_FERTILIZE = 3.5 * HOUR;
    const double TIME_SOW = 10.0 * HOUR;
    const double TIME_ROLL = 6.5 * HOUR;
}

// --- Configuration Struct ---
// Umožňuje přepsat libovolnou hodnotu pro konkrétní běh
struct SimConfig {
    double simulationDuration;
    double probWorkableDay;
    
    // Resources
    int countWorkersShift1;
    int countWorkersShift2;
    int countTractors;
    int countFields;
    
    // Machines
    int countMachineMinTill;
    int countMachineStubble;
    int countMachinePlow;
    int countMachinePrep;
    int countMachineFertilizer;
    int countMachineSower;
    int countMachineRoller;

    // Economics
    double costWorkerPerHour;
    double costTractorPerHour;
    double priceCzkPerTonne;

    // Reliability
    double probTractorUnavailable;

    // Constructor sets defaults
    SimConfig() {
        simulationDuration = Defaults::SIMULATION_DURATION;
        probWorkableDay = Defaults::PROB_WORKABLE_DAY;
        
        countWorkersShift1 = Defaults::COUNT_WORKERS_SHIFT_1;
        countWorkersShift2 = Defaults::COUNT_WORKERS_SHIFT_2;
        countTractors = Defaults::COUNT_TRACTORS;
        countFields = Defaults::COUNT_FIELDS;

        countMachineMinTill = Defaults::COUNT_MACHINE_MIN_TILL;
        countMachineStubble = Defaults::COUNT_MACHINE_STUBBLE;
        countMachinePlow = Defaults::COUNT_MACHINE_PLOW;
        countMachinePrep = Defaults::COUNT_MACHINE_PREP;
        countMachineFertilizer = Defaults::COUNT_MACHINE_FERTILIZER;
        countMachineSower = Defaults::COUNT_MACHINE_SOWER;
        countMachineRoller = Defaults::COUNT_MACHINE_ROLLER;

        costWorkerPerHour = Defaults::COST_WORKER_PER_HOUR;
        costTractorPerHour = Defaults::COST_TRACTOR_PER_HOUR;
        priceCzkPerTonne = Defaults::PRICE_CZK_PER_TONNE;

        probTractorUnavailable = Defaults::PROB_TRACTOR_UNAVAILABLE;
    }
};

// Global pointer to the current configuration used by the simulation
extern SimConfig cfg; 
// Time constants needed globally
extern const int NUM_PHASES;

#endif