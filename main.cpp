#include "simlib.h"
#include "config.h"
#include "farm_classes.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <iomanip>
#include <map>
#include <random>
#include <cstdio>
#include <fstream>
#include <limits>
#include <sys/stat.h>
#include <sys/types.h>

// --- Global Definition ---
SimConfig cfg; 
const int NUM_PHASES = 9;

// Global Variables (Re-initialized per run)
bool IsDayWorkable = true;
double TimeAllReadyForSowing = -1.0; 
int GlobalWorkableDaysCount = 0;

// Global Cost Accumulators
double TotalGlobalWorkerWages = 0;
double TotalGlobalMachineCost = 0;
double TotalGlobalMaterialCost = 0;

// Utilization Accumulators (Manual tracking)
double TotalTractorHours = 0;
double TotalSowerHours = 0;

// Pointers for Dynamic Resources
Store* pTractors = nullptr;
Store* pMachineMinTill = nullptr;
Store* pMachineStubble = nullptr;
Store* pMachinePlow = nullptr;
Store* pMachinePrep = nullptr;
Store* pMachineFertilizer = nullptr;
Store* pMachineSower = nullptr;
Store* pMachineRoller = nullptr;

Store* MachineRequirements[NUM_PHASES];
double PhaseDurations[NUM_PHASES];

std::vector<Field> FarmFields;
std::map<int, double> FieldActiveDurations; 
std::mt19937 cpp_gen(12345);

struct SimStats {
    // Totals
    double totalProfit;
    double totalRevenue;
    double maxPotentialRevenue;
    double totalLoss;
    
    // Expenses
    double costLabor;
    double costMachine;
    double costMaterial;
    double totalExpenses;

    // Operation
    double simEndTime;
    double daysWaitingForWindow; 
    int unfinishedFieldsCount;
    int workableDays;
    bool allFieldsFinished;

    // Utilization Stats
    double avgTractorUtil;
    double avgSowerUtil;
};

// --- Helper Functions ---

void InitLookupArrays() {
    MachineRequirements[PHASE_START] = nullptr; 
    PhaseDurations[PHASE_START] = 0;
    MachineRequirements[PHASE_MIN_TILL] = pMachineFertilizer; 
    PhaseDurations[PHASE_MIN_TILL] = Defaults::TIME_FERTILIZE; 
    MachineRequirements[PHASE_STUBBLE] = pMachinePlow;
    PhaseDurations[PHASE_STUBBLE] = Defaults::TIME_PLOW;
    MachineRequirements[PHASE_PLOW] = pMachinePrep;
    PhaseDurations[PHASE_PLOW] = Defaults::TIME_PREP;
    MachineRequirements[PHASE_PREP] = pMachineFertilizer;
    PhaseDurations[PHASE_PREP] = Defaults::TIME_FERTILIZE;
    
    // Note: The phase named 'FERTILIZE' currently maps to Sower Logic in this model
    MachineRequirements[PHASE_FERTILIZE] = pMachineSower;
    // UPDATED: Use the configurable sowing time instead of static default
    PhaseDurations[PHASE_FERTILIZE] = cfg.sowingTime; 
    
    MachineRequirements[PHASE_SOWING] = pMachineRoller;
    PhaseDurations[PHASE_SOWING] = Defaults::TIME_ROLL;
    MachineRequirements[PHASE_ROLLING] = nullptr; 
    PhaseDurations[PHASE_ROLLING] = 0;
    MachineRequirements[PHASE_DONE] = nullptr;
    PhaseDurations[PHASE_DONE] = 0;
}

double RandomBeta(double alpha, double beta) {
    std::gamma_distribution<double> gamma_alpha(alpha, 1.0);
    std::gamma_distribution<double> gamma_beta(beta, 1.0);
    double x = gamma_alpha(cpp_gen);
    double y = gamma_beta(cpp_gen);
    return x / (x + y);
}

double GetOperationTime(double meanTime) {
    if (meanTime <= 0.001) return 0;
    double minTime = meanTime * 0.75; 
    double maxTime = meanTime * 1.50; 
    double alpha = 2.0;
    double target_ratio = (meanTime - minTime) / (maxTime - minTime);
    double beta = (alpha / target_ratio) - alpha;
    double beta_val = RandomBeta(alpha, beta);
    return minTime + beta_val * (maxTime - minTime);
}

double GetPersistentDuration(int fieldId, double meanTime) {
    if (FieldActiveDurations.find(fieldId) == FieldActiveDurations.end()) {
        FieldActiveDurations[fieldId] = GetOperationTime(meanTime);
    }
    return FieldActiveDurations[fieldId];
}

std::string GetFormattedTime(double t) {
    int totalMinutes = (int)(t * 60);
    int day = totalMinutes / (24 * 60) + 1; 
    int hour = (totalMinutes / 60) % 24;
    int minute = totalMinutes % 60;
    char buffer[50];
    sprintf(buffer, "Day %2d, %02d:%02d", day, hour, minute);
    return std::string(buffer);
}

void LogEvent(std::string actor, std::string action) {
    // Logging disabled for batch runs
}

void AddMaterialCost(Field* field, double costPerHa) {
    double cost = Defaults::FIELD_SIZE_HA * costPerHa;
    field->accumulatedCostCZK += cost;
    TotalGlobalMaterialCost += cost;
}

void UpdateFieldEconomics(Field* field, double workTime, double actualTotalDuration) {
    if (field->currentPhase == PHASE_FERTILIZE) {
        double maxTotalValue = Defaults::FIELD_SIZE_HA * Defaults::YIELD_TONNES_PER_HA * cfg.priceCzkPerTonne;
        double daysLate = 0.0;
        if (Time > Defaults::LATE_SOWING_TIME) {
            daysLate = (Time - Defaults::LATE_SOWING_TIME) / Defaults::DAY;
        }
        double penaltyFactor = 1.0 - (daysLate * Defaults::YIELD_PENALTY_PER_DAY);
        if (penaltyFactor < 0) penaltyFactor = 0;
        double workFraction = workTime / actualTotalDuration; 
        double valueAdded = maxTotalValue * workFraction * penaltyFactor;
        field->accumulatedYieldCZK += valueAdded;
    }

    // Cost calculations
    double workerCost = workTime * cfg.costWorkerPerHour; // Note: Worker cost is tracked in TotalGlobalWorkerWages
    double machineCost = workTime * cfg.costTractorPerHour;
    
    field->accumulatedCostCZK += (workerCost + machineCost);
    
    TotalGlobalMachineCost += machineCost;
}

void CheckSimulationEnd() {
    bool allDone = true;
    for (const auto& field : FarmFields) {
        if (field.currentPhase != PHASE_DONE) {
            allDone = false;
            break;
        }
    }
    if (allDone) {
        Stop(); 
    }
}

void CheckIfAllReadyForSowing() {
    if (TimeAllReadyForSowing != -1.0) return; 

    bool allReady = true;
    for (const auto& f : FarmFields) {
        if (f.currentPhase < PHASE_SOWING) {
            allReady = false;
            break;
        }
    }

    if (allReady) {
        TimeAllReadyForSowing = Time;
    }
}

bool FindBestJob(Field*& outField, Store*& outMachine, double& outDuration) {
    std::vector<Field*> candidates;
    for (auto& field : FarmFields) {
        if (field.currentPhase == PHASE_DONE) continue;
        if (field.isBeingWorkedOn) continue; 
        if (field.currentPhase == PHASE_SOWING && Time < Defaults::SOWING_START_TIME) continue; 
        candidates.push_back(&field);
    }

    if (candidates.empty()) return false;

    std::sort(candidates.begin(), candidates.end(), [](Field* a, Field* b) {
        return a->currentPhase < b->currentPhase;
    });

    for (Field* field : candidates) {
        Store* potentialMachine = nullptr;
        double duration = 0;

        if (field->currentPhase == PHASE_START) {
            if (pMachineMinTill->Capacity() > 0) {
                 field->isMinTillPath = true;
                 potentialMachine = pMachineMinTill;
                 duration = Defaults::TIME_MIN_TILL;
            } else {
                field->isMinTillPath = false;
                potentialMachine = pMachineStubble;
                duration = Defaults::TIME_STUBBLE;
            }
        }
        else {
            int phaseIndex = (int)field->currentPhase;
            if (phaseIndex >= 0 && phaseIndex < NUM_PHASES) {
                potentialMachine = MachineRequirements[phaseIndex];
                duration = PhaseDurations[phaseIndex];
            }
        }

        if (potentialMachine && potentialMachine->Free() > 0) {
            outField = field;
            outMachine = potentialMachine;
            outDuration = duration;
            return true; 
        }
    }
    return false;
}

// --- Processes ---

class WeatherController : public Process {
    void Behavior() {
        while(true) {
            if (Random() < cfg.probWorkableDay) {
                IsDayWorkable = true;
                GlobalWorkableDaysCount++;
            } else {
                IsDayWorkable = false;
            }
            Wait(24.0 * Defaults::HOUR);
        }
    }
};

class Worker : public Process {
    int workerId;
    int shiftType; 

public:
    Worker(int id, int shift) : workerId(id), shiftType(shift) {}

    void Behavior() {
        if (shiftType == 2) {
            Wait(Defaults::SHIFT_2_START_OFFSET);
        }

        while(true) {
            TotalGlobalWorkerWages += Defaults::SHIFT_DURATION * cfg.costWorkerPerHour;

            if (!IsDayWorkable) {
                Wait(Defaults::SHIFT_DURATION);
            } 
            else {
                bool tractorPhysicallyAvailable = (Random() >= cfg.probTractorUnavailable);

                if (tractorPhysicallyAvailable) {
                    if (pTractors->Free() > 0) {
                        Enter(*pTractors, 1);
                        PerformFarmWork();
                        Leave(*pTractors, 1);
                    } else {
                         Wait(Defaults::SHIFT_DURATION);
                    }
                } 
                else {
                    if (Random() < Defaults::PROB_REPAIR_NEEDED) {
                        Wait(Defaults::REPAIR_TIME);
                        Wait(Defaults::SHIFT_DURATION - Defaults::REPAIR_TIME); 
                    } else {
                        Wait(Defaults::SHIFT_DURATION);
                    }
                }
            }
            Wait(Defaults::REST_DURATION);
        }
    }

    void PerformFarmWork() {
        double timeRemainingInShift = Defaults::SHIFT_DURATION;

        while (timeRemainingInShift > 0.1) {
            Field* targetField = nullptr;
            Store* requiredMachine = nullptr;
            double operationFullMeanDuration = 0;

            if (FindBestJob(targetField, requiredMachine, operationFullMeanDuration)) {
                
                targetField->isBeingWorkedOn = true;
                Enter(*requiredMachine, 1);

                double actualTotalDuration = GetPersistentDuration(targetField->id, operationFullMeanDuration);
                double remainingWorkOnField = actualTotalDuration - targetField->workDoneInPhase;
                if (remainingWorkOnField < 0) remainingWorkOnField = 0.1; 

                double workTime = std::min(remainingWorkOnField, timeRemainingInShift);
                Wait(workTime); 

                // --- Utilization Tracking (Manual) ---
                // Worker always has a tractor here
                TotalTractorHours += workTime;

                // Check if specific implement is used
                if (requiredMachine == pMachineSower) {
                    TotalSowerHours += workTime;
                }
                // -------------------------------------

                UpdateFieldEconomics(targetField, workTime, actualTotalDuration);

                timeRemainingInShift -= workTime;
                targetField->workDoneInPhase += workTime;

                if (targetField->workDoneInPhase >= (remainingWorkOnField - 0.01)) {
                    targetField->workDoneInPhase = 0; 
                    FieldActiveDurations.erase(targetField->id);

                    if (targetField->currentPhase == PHASE_PREP || targetField->currentPhase == PHASE_MIN_TILL) {
                        AddMaterialCost(targetField, Defaults::COST_FERTILIZER_PER_HA);
                    }
                    if (targetField->currentPhase == PHASE_FERTILIZE) {
                         AddMaterialCost(targetField, Defaults::COST_SEED_PER_HA);
                    }

                    if (targetField->currentPhase == PHASE_START) {
                        targetField->currentPhase = targetField->isMinTillPath ? PHASE_MIN_TILL : PHASE_STUBBLE;
                    } 
                    else if (targetField->currentPhase == PHASE_MIN_TILL) {
                        targetField->currentPhase = PHASE_FERTILIZE;
                    }
                    else if (targetField->currentPhase == PHASE_SOWING) { 
                        targetField->currentPhase = PHASE_DONE;
                        CheckSimulationEnd();
                    }
                    else {
                        targetField->currentPhase = static_cast<FieldPhase>(targetField->currentPhase + 1);
                    }
                    
                    CheckIfAllReadyForSowing();
                }

                Leave(*requiredMachine, 1);
                targetField->isBeingWorkedOn = false;

            } else {
                Wait(timeRemainingInShift);
                timeRemainingInShift = 0;
            }
        }
    }
};

// --- Execution Logic ---

void CleanupResources() {
    delete pTractors;
    delete pMachineMinTill;
    delete pMachineStubble;
    delete pMachinePlow;
    delete pMachinePrep;
    delete pMachineFertilizer;
    delete pMachineSower;
    delete pMachineRoller;
}

void WriteFinalReport(std::string filename, const SimStats& stats) {
    std::ofstream file(filename);
    if (!file.is_open()) return;

    file << "--- Final Statistics ---\n";
    file << "Simulation End Time: " << GetFormattedTime(Time) << " (" << Time << " raw hours)\n";
    file << "Workable Days: " << stats.workableDays << "\n";
    file << "Waiting for Sowing Window: " << std::fixed << std::setprecision(2) << stats.daysWaitingForWindow << " days\n";
    
    file << "\n=== ECONOMICS REPORT ===\n";
    file << "+------+------------+-------------+-------------+--------------+------------+------------+----------+\n";
    file << "|  ID  |   Status   |   Revenue   |   Max Pot   | Loss (Delay) |  Expenses  | Net Profit |  Margin  |\n";
    file << "+------+------------+-------------+-------------+--------------+------------+------------+----------+\n";
    
    double totalRev = 0;
    double totalMaxRev = 0;
    double totalExp = 0;
    double totalProfit = 0;

    for (const auto& f : FarmFields) {
        std::string status = PhaseNames[f.currentPhase];
        
        double maxRev = Defaults::FIELD_SIZE_HA * Defaults::YIELD_TONNES_PER_HA * cfg.priceCzkPerTonne;
        double loss = maxRev - f.accumulatedYieldCZK;
        double profit = f.accumulatedYieldCZK - f.accumulatedCostCZK;
        double margin = (f.accumulatedYieldCZK > 0) ? (profit / f.accumulatedYieldCZK) * 100.0 : -100.0;

        char buffer[200];
        sprintf(buffer, "| %-4d | %-10s | %11.0f | %11.0f | -%11.0f | %10.0f | %10.0f | %7.1f%% |", 
              f.id, status.c_str(), f.accumulatedYieldCZK, maxRev, loss, f.accumulatedCostCZK, profit, margin);
        file << buffer << "\n";
        
        totalRev += f.accumulatedYieldCZK;
        totalMaxRev += maxRev;
        totalExp += f.accumulatedCostCZK;
        totalProfit += profit;
    }
    file << "+------+------------+-------------+-------------+--------------+------------+------------+----------+\n";
    
    char sumBuffer[200];
    sprintf(sumBuffer, "| %-4s | %-10s | %11.0f | %11.0f | -%11.0f | %10.0f | %10.0f |          |", 
          "SUMS", "", totalRev, totalMaxRev, totalMaxRev - totalRev, totalExp, totalProfit);
    file << sumBuffer << "\n";
          
    file << "+------+------------+-------------+-------------+--------------+------------+------------+----------+\n";
    file << "\n--- Financial Breakdown ---\n";
    file << "Labor Cost:    " << stats.costLabor << " CZK\n";
    file << "Machine Cost:  " << stats.costMachine << " CZK\n";
    file << "Material Cost: " << stats.costMaterial << " CZK\n";
    file << "  (Includes Chem: " << (Defaults::FIELD_SIZE_HA * cfg.countFields * cfg.costChemicalsPerHa) << ")\n";
    file << "  (Includes ExtN: " << (Defaults::FIELD_SIZE_HA * cfg.countFields * cfg.costExtraNitrogenPerHa) << ")\n";
    file.close();
}

SimStats RunSimulation(const SimConfig& runConfig, long seed, std::string reportFilename = "") {
    cfg = runConfig;
    
    // Reset Globals
    TotalGlobalWorkerWages = 0;
    TotalGlobalMachineCost = 0;
    TotalGlobalMaterialCost = 0;
    GlobalWorkableDaysCount = 0;

    // Reset Utilization Counters
    TotalTractorHours = 0;
    TotalSowerHours = 0;

    IsDayWorkable = true;
    TimeAllReadyForSowing = -1.0; 
    FarmFields.clear();
    FieldActiveDurations.clear();
    
    Init(0, cfg.simulationDuration); 
    
    cpp_gen.seed(seed);
    RandomSeed(seed);

    pTractors = new Store("Tractors", cfg.countTractors);
    pMachineMinTill = new Store("MinTill", cfg.countMachineMinTill);
    pMachineStubble = new Store("Stubble", cfg.countMachineStubble);
    pMachinePlow = new Store("Plow", cfg.countMachinePlow);
    pMachinePrep = new Store("Prep", cfg.countMachinePrep);
    pMachineFertilizer = new Store("Fertilizer", cfg.countMachineFertilizer);
    pMachineSower = new Store("Sower", cfg.countMachineSower);
    pMachineRoller = new Store("Roller", cfg.countMachineRoller);

    InitLookupArrays(); 

    for (int i = 0; i < cfg.countFields; i++) {
        FarmFields.push_back(Field(i+1));
    }

    (new WeatherController)->Activate();

    for (int i = 0; i < cfg.countWorkersShift1; i++) {
        (new Worker(i+1, 1))->Activate();
    }
    for (int i = 0; i < cfg.countWorkersShift2; i++) {
        (new Worker(100 + i + 1, 2))->Activate(); 
    }

    Run();

    // --- Post-Simulation Cost Calculations (Realistic Additions) ---
    // Applying "Post-Processing" costs for chemicals and extra fertilization
    // regardless of whether the field was finished, as these are usually applied early/mid season.
    for (auto& field : FarmFields) {
        double chemCost = Defaults::FIELD_SIZE_HA * cfg.costChemicalsPerHa;
        double nitroCost = Defaults::FIELD_SIZE_HA * cfg.costExtraNitrogenPerHa;
        
        double totalExtra = chemCost + nitroCost;
        
        field.accumulatedCostCZK += totalExtra;
        TotalGlobalMaterialCost += totalExtra;
    }

    SimStats stats;
    stats.simEndTime = Time;
    
    stats.totalRevenue = 0;
    stats.maxPotentialRevenue = 0;
    stats.unfinishedFieldsCount = 0;
    stats.allFieldsFinished = true;

    for(const auto& f : FarmFields) {
        stats.totalRevenue += f.accumulatedYieldCZK;
        stats.maxPotentialRevenue += Defaults::FIELD_SIZE_HA * Defaults::YIELD_TONNES_PER_HA * cfg.priceCzkPerTonne;
        
        if(f.currentPhase != PHASE_DONE) {
            stats.allFieldsFinished = false;
            stats.unfinishedFieldsCount++;
        }
    }
    
    stats.totalLoss = stats.maxPotentialRevenue - stats.totalRevenue;

    stats.costLabor = TotalGlobalWorkerWages;
    stats.costMachine = TotalGlobalMachineCost;
    stats.costMaterial = TotalGlobalMaterialCost;
    stats.totalExpenses = stats.costLabor + stats.costMachine + stats.costMaterial;
    
    stats.totalProfit = stats.totalRevenue - stats.totalExpenses; 
    stats.workableDays = GlobalWorkableDaysCount;

    if (TimeAllReadyForSowing != -1.0 && TimeAllReadyForSowing < Defaults::SOWING_START_TIME) {
        stats.daysWaitingForWindow = (Defaults::SOWING_START_TIME - TimeAllReadyForSowing) / Defaults::DAY;
    } else {
        stats.daysWaitingForWindow = 0.0;
    }

    // --- Utilization Calculations (Manual) ---
    // Formula: Total Hours Used / (Total Simulation Time * Number of Machines)
    
    if (cfg.countTractors > 0 && Time > 0) {
        stats.avgTractorUtil = (TotalTractorHours / (Time * cfg.countTractors)) * 100.0;
    } else {
        stats.avgTractorUtil = 0.0;
    }

    if (cfg.countMachineSower > 0 && Time > 0) {
        stats.avgSowerUtil = (TotalSowerHours / (Time * cfg.countMachineSower)) * 100.0;
    } else {
        stats.avgSowerUtil = 0.0;
    }

    if (!reportFilename.empty()) {
        WriteFinalReport(reportFilename, stats);
    }

    CleanupResources();
    return stats;
}

void RunBatch(const SimConfig& batchCfg, int iterations, std::string scenarioName, std::ofstream& csvFile, long baseSeed = 1000) {
    std::cout << "Running Batch: " << scenarioName << " (" << iterations << " iterations)... ";
    std::cout.flush();

    // Accumulators for averages
    double sumProfit = 0;
    double sumRevenue = 0;
    double sumExpenses = 0;
    
    double sumCostLabor = 0;
    double sumCostMachine = 0;
    double sumCostMaterial = 0;
    
    double sumLoss = 0;
    
    double sumTime = 0;
    double sumWait = 0;
    double sumUnfinished = 0;
    int successCount = 0;
    double sumWorkableDays = 0;

    // Resource Utilization Accumulators
    double sumTractorUtil = 0;
    double sumSowerUtil = 0;
    
    double minProfit = std::numeric_limits<double>::max();
    double maxProfit = std::numeric_limits<double>::lowest();

    for (int i = 0; i < iterations; i++) {
        long currentSeed = baseSeed + i;
        std::string reportName = (i == 0) ? "out/detail_" + scenarioName + ".txt" : "";

        SimStats result = RunSimulation(batchCfg, currentSeed, reportName);

        sumProfit += result.totalProfit;
        sumRevenue += result.totalRevenue;
        sumExpenses += result.totalExpenses;
        
        sumCostLabor += result.costLabor;
        sumCostMachine += result.costMachine;
        sumCostMaterial += result.costMaterial;
        
        sumLoss += result.totalLoss;
        
        sumTime += result.simEndTime;
        sumWait += result.daysWaitingForWindow;
        sumUnfinished += result.unfinishedFieldsCount;
        sumWorkableDays += result.workableDays;
        
        sumTractorUtil += result.avgTractorUtil;
        sumSowerUtil += result.avgSowerUtil;

        if (result.allFieldsFinished) successCount++;

        if (result.totalProfit < minProfit) minProfit = result.totalProfit;
        if (result.totalProfit > maxProfit) maxProfit = result.totalProfit;
    }

    // Output to CSV
    csvFile << scenarioName << "," 
            << iterations << "," 
            
            // New Input Columns (Preserving user's new inputs)
            << batchCfg.costChemicalsPerHa << ","
            << batchCfg.costExtraNitrogenPerHa << ","

            // Reliability
            << std::fixed << std::setprecision(1) << (successCount * 100.0 / iterations) << "," 
            << std::setprecision(2) << (sumUnfinished / iterations) << "," 

            // Profits
            << std::setprecision(0) << (sumProfit / iterations) << "," 
            << minProfit << "," 
            << maxProfit << "," 

            // Detail Financials (Averages)
            << (sumRevenue / iterations) << "," 
            << (sumExpenses / iterations) << "," 
            << (sumCostLabor / iterations) << "," 
            << (sumCostMachine / iterations) << "," 
            << (sumCostMaterial / iterations) << "," 
            
            // Losses (The most important risk metric)
            << (sumLoss / iterations) << "," 

            // Utilization Stats (Added)
            << std::setprecision(1) << (sumTractorUtil / iterations) << "%,"
            << (sumSowerUtil / iterations) << "%,"

            // Operational
            << std::setprecision(2) << (sumWait / iterations) << "," 
            << (sumWorkableDays / iterations) << ","
            << (sumTime / iterations) // Duration last, as requested
            << "\n";
            
    csvFile.flush(); // FORCE WRITE TO DISK

    std::cout << "Done.\n";
}

int main() {
    mkdir("out", 0777);

    SetOutput("out/internal_simlib.log"); 

    std::cout << "--- Starting Detailed Batch Analysis ---" << std::endl;

    std::ofstream csvFile("out/test_results.csv");
    
    // Header CSV (Updated with utilization columns)
    csvFile << "Scenario,Iterations,"
            << "Input_ChemCost,Input_ExtraN,"
            << "Success Rate (%),Avg Unfinished Fields,"
            << "Avg Profit,Min Profit,Max Profit,"
            << "Avg Revenue,Avg Total Expenses,Avg Labor Cost,Avg Machine Cost,Avg Material Cost,"
            << "Avg Total Loss (Delay+Unfinished),"
            << "Avg Tractor Util,Avg Sower Util,"
            << "Avg Wait for Window (Days),Avg Workable Days,Avg Duration (Hours)\n";

    int ITERATIONS = 1000;

    // --- 1. Baseline & Bottleneck Identification ---
    // A: Baseline
    SimConfig cfgBaseline; 
    RunBatch(cfgBaseline, ITERATIONS, "A_Baseline", csvFile, 1000);

    // B: High Load (8 Fields) - Stress test to find bottlenecks
    SimConfig cfgHighLoad;
    cfgHighLoad.countFields = 8;
    RunBatch(cfgHighLoad, ITERATIONS, "B_High_Load", csvFile, 2000);

    // --- 2. Sowing Machine Analysis (Addressing Question 1.1) ---
    // C: Double Sower (2 Machines) - Running on High Load to see if it fixes B
    SimConfig cfgDoubleSower;
    cfgDoubleSower.countFields = 8;
    cfgDoubleSower.countMachineSower = 2;
    RunBatch(cfgDoubleSower, ITERATIONS, "C_Double_Sower", csvFile, 3000);

    // D: Faster Sower (30% faster) - Running on High Load
    SimConfig cfgFastSower;
    cfgFastSower.countFields = 8;
    cfgFastSower.sowingTime = Defaults::TIME_SOW * 0.7; // 70% of original time
    RunBatch(cfgFastSower, ITERATIONS, "D_Faster_Sower", csvFile, 4000);

    // --- 3. Line Configuration & Labor (Addressing Labor impact) ---
    // E: Lean Operation (Cost cutting) - 1 Tractor on standard 4 fields
    SimConfig cfgLean;
    cfgLean.countTractors = 1;
    RunBatch(cfgLean, ITERATIONS, "E_Lean_Ops", csvFile, 5000);

    // F: Heavy Shift (Capacity Boost) - Addressing "prodloužení směny"
    // Using 8 fields. Increasing Shift 2 workers to match Shift 1, and adding a tractor.
    SimConfig cfgHeavy;
    cfgHeavy.countFields = 8;
    cfgHeavy.countWorkersShift2 = 2; // Equal to shift 1
    cfgHeavy.countTractors = 3;      // More tractors to support more workers
    RunBatch(cfgHeavy, ITERATIONS, "F_Heavy_Shift", csvFile, 6000);

    // --- 4. Stochastic Events (Risk Analysis) ---
    // G: Critical Weather
    SimConfig cfgWeather;
    cfgWeather.probWorkableDay = 0.25; // Drastic reduction from 0.40
    RunBatch(cfgWeather, ITERATIONS, "G_Bad_Weather", csvFile, 7000);

    // H: High Failure Rate (Machine reliability)
    SimConfig cfgFail;
    cfgFail.probTractorUnavailable = 0.60; // High probability tractor is gone (and thus worker idle)
    RunBatch(cfgFail, ITERATIONS, "H_Machine_Fail", csvFile, 8000);

    // --- 5. Min-Till Analysis (The New Bottleneck Question) ---
    // I: Min-Till High Load
    // Can Min-Till save the day for 8 fields without buying extra tractors?
    SimConfig cfgMinTill;
    cfgMinTill.countFields = 8;
    cfgMinTill.countMachineMinTill = 1; // Enables Min-Till path (skips plow)
    RunBatch(cfgMinTill, ITERATIONS, "I_MinTill_HighLoad", csvFile, 9000);

    // J: Min-Till Lean
    // Can we run 8 fields with just 1 tractor if we skip plowing?
    SimConfig cfgMinTillLean;
    cfgMinTillLean.countFields = 8;
    cfgMinTillLean.countMachineMinTill = 1;
    cfgMinTillLean.countTractors = 1; 
    RunBatch(cfgMinTillLean, ITERATIONS, "J_MinTill_Lean", csvFile, 10000);

    // --- 6. Incremental Scaling & Isolation ---
    // K: Medium Load (6 Fields) - Finding the tipping point between 4 and 8
    SimConfig cfgMedium;
    cfgMedium.countFields = 6;
    RunBatch(cfgMedium, ITERATIONS, "K_Medium_Load", csvFile, 11000);

    // L: Tractor Only Boost (8 Fields) - 3 Tractors, Default Workers
    SimConfig cfgTractorsOnly;
    cfgTractorsOnly.countFields = 8;
    cfgTractorsOnly.countTractors = 3;
    RunBatch(cfgTractorsOnly, ITERATIONS, "L_More_Tractors", csvFile, 12000);

    // M: Worker Only Boost (8 Fields) - 2 Tractors, Full Shift 2
    SimConfig cfgWorkersOnly;
    cfgWorkersOnly.countFields = 8;
    cfgWorkersOnly.countWorkersShift2 = 2;
    RunBatch(cfgWorkersOnly, ITERATIONS, "M_More_Workers", csvFile, 13000);

    // --- 7. Resilience Stress Tests ---
    // N: Heavy Shift vs Bad Weather - Can the "F" config survive 25% weather?
    SimConfig cfgHeavyWeather;
    cfgHeavyWeather.countFields = 8;
    cfgHeavyWeather.countWorkersShift2 = 2;
    cfgHeavyWeather.countTractors = 3;
    cfgHeavyWeather.probWorkableDay = 0.25;
    RunBatch(cfgHeavyWeather, ITERATIONS, "N_Heavy_Resilience", csvFile, 14000);

    // O: Min-Till vs Bad Weather - Is speed the answer to bad weather?
    SimConfig cfgMinTillWeather;
    cfgMinTillWeather.countFields = 8;
    cfgMinTillWeather.countMachineMinTill = 1;
    cfgMinTillWeather.probWorkableDay = 0.25;
    RunBatch(cfgMinTillWeather, ITERATIONS, "O_MinTill_Resilience", csvFile, 15000);

    // --- 8. Extreme & Special Cases ---
    // P: The "Super Farm" - 12 Fields, High Tech, High Labor
    SimConfig cfgSuperFarm;
    cfgSuperFarm.countFields = 12;
    cfgSuperFarm.countTractors = 4;
    cfgSuperFarm.countWorkersShift2 = 2;
    cfgSuperFarm.countMachineMinTill = 1; 
    RunBatch(cfgSuperFarm, ITERATIONS, "P_Super_Farm", csvFile, 16000);

    // Q: Sower Redemption - 8 Fields, Bad Weather, 2 Sowers
    // Does the extra sower help when the weather window is tiny?
    SimConfig cfgSowerRedemption;
    cfgSowerRedemption.countFields = 8;
    cfgSowerRedemption.probWorkableDay = 0.25;
    cfgSowerRedemption.countMachineSower = 2;
    RunBatch(cfgSowerRedemption, ITERATIONS, "Q_Sower_Redemption", csvFile, 17000);

    csvFile.close();
    std::cout << "\nAnalysis Complete. Check 'out/test_results.csv' for full details.\n";

    return 0;
}