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

// RNG Generator
std::mt19937 cpp_gen(12345);

// ============================================================================
//  HELPER FUNCTIONS
// ============================================================================

void InitLookupArrays(SimulationContext& ctx) {
    // Reset pointers
    for(int i=0; i<NUM_PHASES; ++i) ctx.machineRequirements[i] = nullptr;

    // Mapping Phases to Machines
    ctx.machineRequirements[PHASE_MIN_TILL]  = ctx.pMachineFertilizer; // Assuming logic from original code
    ctx.phaseDurations[PHASE_MIN_TILL]       = Defaults::TIME_FERTILIZE; 

    ctx.machineRequirements[PHASE_STUBBLE]   = ctx.pMachinePlow;
    ctx.phaseDurations[PHASE_STUBBLE]        = Defaults::TIME_PLOW;
    
    ctx.machineRequirements[PHASE_PLOW]      = ctx.pMachinePrep;
    ctx.phaseDurations[PHASE_PLOW]           = Defaults::TIME_PREP;
    
    ctx.machineRequirements[PHASE_PREP]      = ctx.pMachineFertilizer;
    ctx.phaseDurations[PHASE_PREP]           = Defaults::TIME_FERTILIZE;
    
    // Note: The phase named 'FERTILIZE' currently maps to Sower Logic in this model
    ctx.machineRequirements[PHASE_FERTILIZE] = ctx.pMachineSower;
    ctx.phaseDurations[PHASE_FERTILIZE]      = ctx.cfg.sowingTime; 
    
    ctx.machineRequirements[PHASE_SOWING]    = ctx.pMachineRoller;
    ctx.phaseDurations[PHASE_SOWING]         = Defaults::TIME_ROLL;
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

double GetPersistentDuration(SimulationContext& ctx, int fieldId, double meanTime) {
    if (ctx.fieldActiveDurations.find(fieldId) == ctx.fieldActiveDurations.end()) {
        ctx.fieldActiveDurations[fieldId] = GetOperationTime(meanTime);
    }
    return ctx.fieldActiveDurations[fieldId];
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

void AddMaterialCost(SimulationContext& ctx, Field* field, double costPerHa) {
    double cost = Defaults::FIELD_SIZE_HA * costPerHa;
    field->accumulatedCostCZK += cost;
    ctx.totalMaterialCost += cost;
}

void UpdateFieldEconomics(SimulationContext& ctx, Field* field, double workTime, double actualTotalDuration) {
    if (field->currentPhase == PHASE_FERTILIZE) {
        double maxTotalValue = Defaults::FIELD_SIZE_HA * Defaults::YIELD_TONNES_PER_HA * ctx.cfg.priceCzkPerTonne;
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
    double workerCost = workTime * ctx.cfg.costWorkerPerHour; // Tracked globally in totalWorkerWages
    double machineCost = workTime * ctx.cfg.costTractorPerHour;
    
    field->accumulatedCostCZK += (workerCost + machineCost);
    ctx.totalMachineCost += machineCost;
}

void CheckSimulationEnd(SimulationContext& ctx) {
    bool allDone = true;
    for (const auto& field : ctx.farmFields) {
        if (field.currentPhase != PHASE_DONE) {
            allDone = false;
            break;
        }
    }
    if (allDone) {
        Stop(); 
    }
}

void CheckIfAllReadyForSowing(SimulationContext& ctx) {
    if (ctx.timeAllReadyForSowing != -1.0) return; 

    bool allReady = true;
    for (const auto& f : ctx.farmFields) {
        if (f.currentPhase < PHASE_SOWING) {
            allReady = false;
            break;
        }
    }

    if (allReady) {
        ctx.timeAllReadyForSowing = Time;
    }
}

bool FindBestJob(SimulationContext& ctx, Field*& outField, Store*& outMachine, double& outDuration) {
    std::vector<Field*> candidates;
    for (auto& field : ctx.farmFields) {
        if (field.currentPhase == PHASE_DONE) continue;
        if (field.isBeingWorkedOn) continue; 
        if (field.currentPhase == PHASE_SOWING && Time < Defaults::SOWING_START_TIME) continue; 
        candidates.push_back(&field);
    }

    if (candidates.empty()) return false;

    // Prioritize fields in earlier phases (phase-based priority)
    std::sort(candidates.begin(), candidates.end(), [](Field* a, Field* b) {
        return a->currentPhase < b->currentPhase;
    });

    for (Field* field : candidates) {
        Store* potentialMachine = nullptr;
        double duration = 0;

        if (field->currentPhase == PHASE_START) {
            if (ctx.pMachineMinTill->Capacity() > 0) {
                 field->isMinTillPath = true;
                 potentialMachine = ctx.pMachineMinTill;
                 duration = Defaults::TIME_MIN_TILL;
            } else {
                field->isMinTillPath = false;
                potentialMachine = ctx.pMachineStubble;
                duration = Defaults::TIME_STUBBLE;
            }
        }
        else {
            int phaseIndex = (int)field->currentPhase;
            if (phaseIndex >= 0 && phaseIndex < NUM_PHASES) {
                potentialMachine = ctx.machineRequirements[phaseIndex];
                duration = ctx.phaseDurations[phaseIndex];
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

// ============================================================================
//  PROCESSES
// ============================================================================

class WeatherController : public Process {
    SimulationContext& ctx;
public:
    WeatherController(SimulationContext& context) : ctx(context) {}
    
    void Behavior() override {
        while(true) {
            if (Random() < ctx.cfg.probWorkableDay) {
                ctx.isDayWorkable = true;
                ctx.globalWorkableDaysCount++;
            } else {
                ctx.isDayWorkable = false;
            }
            Wait(24.0 * Defaults::HOUR);
        }
    }
};

class Worker : public Process {
    int workerId;
    int shiftType; 
    SimulationContext& ctx;

public:
    Worker(int id, int shift, SimulationContext& context) 
        : workerId(id), shiftType(shift), ctx(context) {}

    void Behavior() override {
        if (shiftType == 2) {
            Wait(Defaults::SHIFT_2_START_OFFSET);
        }

        while(true) {
            ctx.totalWorkerWages += Defaults::SHIFT_DURATION * ctx.cfg.costWorkerPerHour;

            if (!ctx.isDayWorkable) {
                Wait(Defaults::SHIFT_DURATION);
            } 
            else {
                bool tractorPhysicallyAvailable = (Random() >= ctx.cfg.probTractorUnavailable);

                if (tractorPhysicallyAvailable) {
                    if (ctx.pTractors->Free() > 0) {
                        Enter(*ctx.pTractors, 1);
                        PerformFarmWork();
                        Leave(*ctx.pTractors, 1);
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

            if (FindBestJob(ctx, targetField, requiredMachine, operationFullMeanDuration)) {
                
                targetField->isBeingWorkedOn = true;
                Enter(*requiredMachine, 1);

                double actualTotalDuration = GetPersistentDuration(ctx, targetField->id, operationFullMeanDuration);
                double remainingWorkOnField = actualTotalDuration - targetField->workDoneInPhase;
                if (remainingWorkOnField < 0) remainingWorkOnField = 0.1; 

                double workTime = std::min(remainingWorkOnField, timeRemainingInShift);
                Wait(workTime); 

                // --- Utilization Tracking ---
                ctx.totalTractorHours += workTime;
                if (requiredMachine == ctx.pMachineSower) {
                    ctx.totalSowerHours += workTime;
                }

                UpdateFieldEconomics(ctx, targetField, workTime, actualTotalDuration);

                timeRemainingInShift -= workTime;
                targetField->workDoneInPhase += workTime;

                // Job Completion
                if (targetField->workDoneInPhase >= (remainingWorkOnField - 0.01)) {
                    targetField->workDoneInPhase = 0; 
                    ctx.fieldActiveDurations.erase(targetField->id);

                    if (targetField->currentPhase == PHASE_PREP || targetField->currentPhase == PHASE_MIN_TILL) {
                        AddMaterialCost(ctx, targetField, Defaults::COST_FERTILIZER_PER_HA);
                    }
                    if (targetField->currentPhase == PHASE_FERTILIZE) {
                         AddMaterialCost(ctx, targetField, Defaults::COST_SEED_PER_HA);
                    }

                    // Phase Transition Logic
                    if (targetField->currentPhase == PHASE_START) {
                        targetField->currentPhase = targetField->isMinTillPath ? PHASE_MIN_TILL : PHASE_STUBBLE;
                    } 
                    else if (targetField->currentPhase == PHASE_MIN_TILL) {
                        targetField->currentPhase = PHASE_FERTILIZE;
                    }
                    else if (targetField->currentPhase == PHASE_SOWING) { 
                        targetField->currentPhase = PHASE_DONE;
                        CheckSimulationEnd(ctx);
                    }
                    else {
                        targetField->currentPhase = static_cast<FieldPhase>(targetField->currentPhase + 1);
                    }
                    
                    CheckIfAllReadyForSowing(ctx);
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

// ============================================================================
//  MANAGEMENT
// ============================================================================

void CleanupResources(SimulationContext& ctx) {
    delete ctx.pTractors;
    delete ctx.pMachineMinTill;
    delete ctx.pMachineStubble;
    delete ctx.pMachinePlow;
    delete ctx.pMachinePrep;
    delete ctx.pMachineFertilizer;
    delete ctx.pMachineSower;
    delete ctx.pMachineRoller;
}

void WriteFinalReport(const SimulationContext& ctx, std::string filename, const SimStats& stats) {
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

    for (const auto& f : ctx.farmFields) {
        std::string status = PhaseNames[f.currentPhase];
        
        double maxRev = Defaults::FIELD_SIZE_HA * Defaults::YIELD_TONNES_PER_HA * ctx.cfg.priceCzkPerTonne;
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
    file << "  (Includes Chem: " << (Defaults::FIELD_SIZE_HA * ctx.cfg.countFields * ctx.cfg.costChemicalsPerHa) << ")\n";
    file << "  (Includes ExtN: " << (Defaults::FIELD_SIZE_HA * ctx.cfg.countFields * ctx.cfg.costExtraNitrogenPerHa) << ")\n";
    file.close();
}

SimStats RunSimulation(const SimConfig& runConfig, long seed, std::string reportFilename = "") {
    SimulationContext ctx;
    ctx.cfg = runConfig;
    
    Init(0, ctx.cfg.simulationDuration); 
    
    cpp_gen.seed(seed);
    RandomSeed(seed);

    // Initialize Resources
    ctx.pTractors          = new Store("Tractors", ctx.cfg.countTractors);
    ctx.pMachineMinTill    = new Store("MinTill", ctx.cfg.countMachineMinTill);
    ctx.pMachineStubble    = new Store("Stubble", ctx.cfg.countMachineStubble);
    ctx.pMachinePlow       = new Store("Plow", ctx.cfg.countMachinePlow);
    ctx.pMachinePrep       = new Store("Prep", ctx.cfg.countMachinePrep);
    ctx.pMachineFertilizer = new Store("Fertilizer", ctx.cfg.countMachineFertilizer);
    ctx.pMachineSower      = new Store("Sower", ctx.cfg.countMachineSower);
    ctx.pMachineRoller     = new Store("Roller", ctx.cfg.countMachineRoller);

    InitLookupArrays(ctx); 

    // Initialize Fields
    for (int i = 0; i < ctx.cfg.countFields; i++) {
        ctx.farmFields.push_back(Field(i+1));
    }

    // Activate Processes
    (new WeatherController(ctx))->Activate();

    for (int i = 0; i < ctx.cfg.countWorkersShift1; i++) {
        (new Worker(i+1, 1, ctx))->Activate();
    }
    for (int i = 0; i < ctx.cfg.countWorkersShift2; i++) {
        (new Worker(100 + i + 1, 2, ctx))->Activate(); 
    }

    // Run Simulation
    Run();

    // --- Post-Simulation Processing ---
    
    // Apply "Post-Processing" costs (chemicals, extra N)
    for (auto& field : ctx.farmFields) {
        double chemCost = Defaults::FIELD_SIZE_HA * ctx.cfg.costChemicalsPerHa;
        double nitroCost = Defaults::FIELD_SIZE_HA * ctx.cfg.costExtraNitrogenPerHa;
        double totalExtra = chemCost + nitroCost;
        
        field.accumulatedCostCZK += totalExtra;
        ctx.totalMaterialCost += totalExtra;
    }

    SimStats stats;
    stats.simEndTime = Time;
    
    stats.totalRevenue = 0;
    stats.maxPotentialRevenue = 0;
    stats.unfinishedFieldsCount = 0;
    stats.allFieldsFinished = true;

    for(const auto& f : ctx.farmFields) {
        stats.totalRevenue += f.accumulatedYieldCZK;
        stats.maxPotentialRevenue += Defaults::FIELD_SIZE_HA * Defaults::YIELD_TONNES_PER_HA * ctx.cfg.priceCzkPerTonne;
        
        if(f.currentPhase != PHASE_DONE) {
            stats.allFieldsFinished = false;
            stats.unfinishedFieldsCount++;
        }
    }
    
    stats.totalLoss = stats.maxPotentialRevenue - stats.totalRevenue;

    stats.costLabor = ctx.totalWorkerWages;
    stats.costMachine = ctx.totalMachineCost;
    stats.costMaterial = ctx.totalMaterialCost;
    stats.totalExpenses = stats.costLabor + stats.costMachine + stats.costMaterial;
    
    stats.totalProfit = stats.totalRevenue - stats.totalExpenses; 
    stats.workableDays = ctx.globalWorkableDaysCount;

    if (ctx.timeAllReadyForSowing != -1.0 && ctx.timeAllReadyForSowing < Defaults::SOWING_START_TIME) {
        stats.daysWaitingForWindow = (Defaults::SOWING_START_TIME - ctx.timeAllReadyForSowing) / Defaults::DAY;
    } else {
        stats.daysWaitingForWindow = 0.0;
    }

    // Utilization Logic
    if (ctx.cfg.countTractors > 0 && Time > 0) {
        stats.avgTractorUtil = (ctx.totalTractorHours / (Time * ctx.cfg.countTractors)) * 100.0;
    } else {
        stats.avgTractorUtil = 0.0;
    }

    if (ctx.cfg.countMachineSower > 0 && Time > 0) {
        stats.avgSowerUtil = (ctx.totalSowerHours / (Time * ctx.cfg.countMachineSower)) * 100.0;
    } else {
        stats.avgSowerUtil = 0.0;
    }

    if (!reportFilename.empty()) {
        WriteFinalReport(ctx, reportFilename, stats);
    }

    CleanupResources(ctx);
    return stats;
}

void RunBatch(const SimConfig& batchCfg, int iterations, std::string scenarioName, std::ofstream& csvFile, long baseSeed = 1000) {
    std::cout << "Running Batch: " << scenarioName << " (" << iterations << " iterations)... ";
    std::cout.flush();

    // Accumulators for averages
    double sumProfit = 0, sumRevenue = 0, sumExpenses = 0;
    double sumCostLabor = 0, sumCostMachine = 0, sumCostMaterial = 0;
    double sumLoss = 0;
    double sumTime = 0, sumWait = 0, sumUnfinished = 0;
    double sumWorkableDays = 0;
    double sumTractorUtil = 0, sumSowerUtil = 0;
    int successCount = 0;
    
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
            
            // Losses
            << (sumLoss / iterations) << "," 

            // Utilization Stats
            << std::setprecision(1) << (sumTractorUtil / iterations) << "%,"
            << (sumSowerUtil / iterations) << "%,"

            // Operational
            << std::setprecision(2) << (sumWait / iterations) << "," 
            << (sumWorkableDays / iterations) << ","
            << (sumTime / iterations)
            << "\n";
            
    csvFile.flush(); 

    std::cout << "Done.\n";
}

int main() {
    mkdir("out", 0777);
    SetOutput("out/internal_simlib.log"); 

    std::cout << "--- Starting Detailed Batch Analysis ---" << std::endl;

    std::ofstream csvFile("out/test_results.csv");
    
    // CSV Header
    csvFile << "Scenario,Iterations,"
            << "Input_ChemCost,Input_ExtraN,"
            << "Success Rate (%),Avg Unfinished Fields,"
            << "Avg Profit,Min Profit,Max Profit,"
            << "Avg Revenue,Avg Total Expenses,Avg Labor Cost,Avg Machine Cost,Avg Material Cost,"
            << "Avg Total Loss (Delay+Unfinished),"
            << "Avg Tractor Util,Avg Sower Util,"
            << "Avg Wait for Window (Days),Avg Workable Days,Avg Duration (Hours)\n";

    int ITERATIONS = 1000;

    // --- 1. Baseline & Scaling Analysis ---
    // Objective: Observe system behavior as field count increases (4 -> 6 -> 8).
    
    SimConfig cfgBaseline; 
    RunBatch(cfgBaseline, ITERATIONS, "A_Baseline_4F", csvFile, 1000);

    SimConfig cfgMedium;
    cfgMedium.countFields = 6;
    RunBatch(cfgMedium, ITERATIONS, "B_Medium_6F", csvFile, 2000);

    SimConfig cfgHighLoad;
    cfgHighLoad.countFields = 8;
    RunBatch(cfgHighLoad, ITERATIONS, "C_High_8F", csvFile, 3000);

    // --- 2. Bottleneck Identification (Classic Tech) ---
    // Objective: Identify which machine causes the failure at 8 fields.
    
    // Hypothesis A: Sower is the bottleneck
    SimConfig cfgDoubleSower;
    cfgDoubleSower.countFields = 8;
    cfgDoubleSower.countMachineSower = 2;
    RunBatch(cfgDoubleSower, ITERATIONS, "D_Double_Sower", csvFile, 4000);

    // Hypothesis B: Plow and Prep are the bottlenecks (New Test)
    SimConfig cfgDoublePlowPrep;
    cfgDoublePlowPrep.countFields = 8;
    cfgDoublePlowPrep.countMachinePlow = 2;
    cfgDoublePlowPrep.countMachinePrep = 2;
    RunBatch(cfgDoublePlowPrep, ITERATIONS, "E_Double_PlowPrep", csvFile, 5000);

    // --- 3. Min-Till Analysis ---
    // Objective: Evaluate alternative soil processing technology.
    
    SimConfig cfgMinTill;
    cfgMinTill.countFields = 8;
    cfgMinTill.countMachineMinTill = 1; 
    RunBatch(cfgMinTill, ITERATIONS, "F_MinTill_High", csvFile, 6000);

    // Realistic Min-Till Cost (New Test)
    // Min-Till saves fuel/time but requires more chemicals (herbicides).
    SimConfig cfgMinTillReal;
    cfgMinTillReal.countFields = 8;
    cfgMinTillReal.countMachineMinTill = 1;
    cfgMinTillReal.costChemicalsPerHa = Defaults::COST_CHEMICALS_PER_HA * 1.5; // +50% chemical cost
    RunBatch(cfgMinTillReal, ITERATIONS, "G_MinTill_RealCost", csvFile, 7000);

    // --- 4. Line Configuration & Labor ---
    // Objective: Test resource allocation limits (Lean vs Heavy).
    
    SimConfig cfgLean;
    cfgLean.countTractors = 1;
    RunBatch(cfgLean, ITERATIONS, "H_Lean_Ops", csvFile, 8000);

    SimConfig cfgHeavy;
    cfgHeavy.countFields = 8;
    cfgHeavy.countWorkersShift2 = 2; 
    cfgHeavy.countTractors = 3;      
    RunBatch(cfgHeavy, ITERATIONS, "I_Heavy_Shift", csvFile, 9000);

    // --- 5. Stochastic Events (Risk Analysis) ---
    // Objective: Test resilience against weather and failures.
    
    SimConfig cfgWeather;
    cfgWeather.probWorkableDay = 0.25; 
    RunBatch(cfgWeather, ITERATIONS, "J_Risk_Weather", csvFile, 10000);

    SimConfig cfgFail;
    cfgFail.probTractorUnavailable = 0.60; 
    RunBatch(cfgFail, ITERATIONS, "K_Risk_MachineFail", csvFile, 11000);

    // --- 6. Extreme & Special Cases ---
    
    SimConfig cfgSuperFarm;
    cfgSuperFarm.countFields = 12;
    cfgSuperFarm.countTractors = 4;
    cfgSuperFarm.countWorkersShift2 = 2;
    cfgSuperFarm.countMachineMinTill = 1; 
    RunBatch(cfgSuperFarm, ITERATIONS, "L_Super_Farm", csvFile, 12000);

    csvFile.close();
    std::cout << "\nAnalysis Complete. Check 'out/test_results.csv' for full details.\n";

    return 0;
}