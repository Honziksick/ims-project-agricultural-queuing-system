#ifndef CONFIG_H
#define CONFIG_H

#include "simlib.h"

// ============================================================================
//  DEFAULT CONSTANTS
// ============================================================================

namespace Defaults {
    // --- Time Units ---
    constexpr double HOUR = 1.0;
    constexpr double DAY = 24.0 * HOUR;
    constexpr double SIMULATION_DURATION = 60.0 * DAY;

    // --- Sowing Windows ---
    constexpr double PROB_WORKABLE_DAY = 0.40;
    constexpr double SOWING_START_TIME = 19.0 * DAY; 
    constexpr double LATE_SOWING_TIME = (19.0 + 21.0) * DAY;

    // --- Field & Market ---
    constexpr double FIELD_SIZE_HA = 15.0;
    constexpr double YIELD_TONNES_PER_HA = 6.5;
    constexpr double PRICE_CZK_PER_TONNE = 4800.0;
    constexpr double YIELD_PENALTY_PER_DAY = 0.01;

    // --- Operational Costs ---
    constexpr double COST_WORKER_PER_HOUR = 250.0;
    constexpr double COST_TRACTOR_PER_HOUR = 900.0;
    constexpr double COST_SEED_PER_HA = 2200.0;
    constexpr double COST_FERTILIZER_PER_HA = 3500.0;

    // --- Post-Process Costs ---
    // Costs for plant protection (sprays) and additional nutrition
    constexpr double COST_CHEMICALS_PER_HA = 6000.0;      
    constexpr double COST_EXTRA_NITROGEN_PER_HA = 3000.0; 

    // --- Resource Counts ---
    constexpr int COUNT_WORKERS_SHIFT_1 = 2;
    constexpr int COUNT_WORKERS_SHIFT_2 = 0;
    constexpr int COUNT_TRACTORS = 2;
    constexpr int COUNT_FIELDS = 4;

    // --- Machine Counts ---
    constexpr int COUNT_MACHINE_MIN_TILL = 0; 
    constexpr int COUNT_MACHINE_STUBBLE = 1;
    constexpr int COUNT_MACHINE_PLOW = 1;
    constexpr int COUNT_MACHINE_PREP = 1;
    constexpr int COUNT_MACHINE_FERTILIZER = 1;
    constexpr int COUNT_MACHINE_SOWER = 1;
    constexpr int COUNT_MACHINE_ROLLER = 1;

    // --- Durations & Probabilities ---
    constexpr double SHIFT_DURATION = 10.0 * HOUR;
    constexpr double REST_DURATION = 24 * HOUR - SHIFT_DURATION;
    constexpr double SHIFT_2_START_OFFSET = SHIFT_DURATION + 0.1 * HOUR; 

    constexpr double PROB_TRACTOR_UNAVAILABLE = 0.30;
    constexpr double PROB_REPAIR_NEEDED = 0.28;
    constexpr double REPAIR_TIME = 4.0 * HOUR;

    // --- Operation Times (Mean) ---
    constexpr double TIME_MIN_TILL = 5.0 * HOUR;
    constexpr double TIME_STUBBLE = 9.5 * HOUR;
    constexpr double TIME_PLOW = 15.5 * HOUR;
    constexpr double TIME_PREP = 8.0 * HOUR;
    constexpr double TIME_FERTILIZE = 3.5 * HOUR;
    constexpr double TIME_SOW = 10.0 * HOUR; 
    constexpr double TIME_ROLL = 6.5 * HOUR;
}

// ============================================================================
//  SIMULATION CONFIGURATION
// ============================================================================

struct SimConfig {
    double simulationDuration;
    double probWorkableDay;
    
    // --- Resources ---
    int countWorkersShift1;
    int countWorkersShift2;
    int countTractors;
    int countFields;
    
    // --- Machines ---
    int countMachineMinTill;
    int countMachineStubble;
    int countMachinePlow;
    int countMachinePrep;
    int countMachineFertilizer;
    int countMachineSower;
    int countMachineRoller;

    // --- Economics ---
    double costWorkerPerHour;
    double costTractorPerHour;
    double priceCzkPerTonne;
    double costChemicalsPerHa;
    double costExtraNitrogenPerHa;

    // --- Reliability ---
    double probTractorUnavailable;

    // --- Variable Durations ---
    double sowingTime;

    // Constructor: Initialize with Defaults
    SimConfig() {
        simulationDuration      = Defaults::SIMULATION_DURATION;
        probWorkableDay         = Defaults::PROB_WORKABLE_DAY;
        
        countWorkersShift1      = Defaults::COUNT_WORKERS_SHIFT_1;
        countWorkersShift2      = Defaults::COUNT_WORKERS_SHIFT_2;
        countTractors           = Defaults::COUNT_TRACTORS;
        countFields             = Defaults::COUNT_FIELDS;

        countMachineMinTill     = Defaults::COUNT_MACHINE_MIN_TILL;
        countMachineStubble     = Defaults::COUNT_MACHINE_STUBBLE;
        countMachinePlow        = Defaults::COUNT_MACHINE_PLOW;
        countMachinePrep        = Defaults::COUNT_MACHINE_PREP;
        countMachineFertilizer  = Defaults::COUNT_MACHINE_FERTILIZER;
        countMachineSower       = Defaults::COUNT_MACHINE_SOWER;
        countMachineRoller      = Defaults::COUNT_MACHINE_ROLLER;

        costWorkerPerHour       = Defaults::COST_WORKER_PER_HOUR;
        costTractorPerHour      = Defaults::COST_TRACTOR_PER_HOUR;
        priceCzkPerTonne        = Defaults::PRICE_CZK_PER_TONNE;

        costChemicalsPerHa      = Defaults::COST_CHEMICALS_PER_HA;
        costExtraNitrogenPerHa  = Defaults::COST_EXTRA_NITROGEN_PER_HA;

        probTractorUnavailable  = Defaults::PROB_TRACTOR_UNAVAILABLE;
        sowingTime              = Defaults::TIME_SOW;
    }
};

#endif // CONFIG_H