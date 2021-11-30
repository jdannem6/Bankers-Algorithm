// CS44001: Operating Systems
// banker.cpp
// Written by Justin Dannemiller
// 11/18/2021
// This program implements the Banker's Algorithm, demonstrating a method by
// which operating system deadlock can be avoided by keeping the system
// in a state where deadlock is not possible.

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using std::cin; using std::cout; using std::endl;
using std::ifstream;
using std::vector; using std::string;

enum class State {finished, running};

// Represents a process within the system. Has two states: finished and 
// running. All programs start in the running state and reach the
// finished state once they have been fully allocated all their needed
// resources to complete their task
class Process {
public:
    // Initializes process state and the resource vectors
    Process(vector<int> allocated, vector<int> maximum, vector<int>* available):
        state_(State::running), allocatedResources_(allocated),
        maximumResources_(maximum), availableResources_(available) 
        {setNeeded();}
    
    // Returns process's state
    State getState() {return state_;}
    // Returns true if safe for process to request neededResources
    bool safeToRequest() const; 
    void requestResources(); // requests resources from what is available
    
    // Releases allocated resources once task is finished; state to finished
    void finishTask();


private:
    void setNeeded(); // calculates and sets needed_resources


    State state_; 
    vector<int> allocatedResources_; // number of resources currently allocated
    vector<int> maximumResources_; // Max. numberof resources 
    vector<int> neededResources_; // additional resources that the process must
                              // be allocated to complete its task

// resources available for all processes to claim; mutable and shared by all
// instances of Process class
vector<int>* availableResources_;



};

// Calculates and sets the number of each resource type needed by the process. 
// needed[i] = max[i] - allocated[i]
void Process::setNeeded() {
    // For each resource type, calculate the number of additional resources needed 
    for (int i = 0; i < allocatedResources_.size(); ++i) {
        neededResources_.push_back(maximumResources_[i] - 
                                   allocatedResources_[i]);
    }
}

// Returns true if the process requesting the additional resources it needs 
// would leave the system in a safe state; false if it would leave the 
// system in an unsafe state
bool Process::safeToRequest() const {
    // If needed[i] <= available[i] for all resources i, the request is safe
    for (int i = 0; i < neededResources_.size(); ++i) {
        // Once needed exceeds available for any i, the system would be unsafe
        if (neededResources_[i] > availableResources_->at(i))
            return false;
    }
    // needed[i] <= available[i] for all i, so the request would be safe
    return true;
}

// Process requests additional resources specified by neededResources to 
// complete its task; increases allocated resources, decreases resources
// available to all other processes
void Process::requestResources() {
    // Can only request if safe
    if (safeToRequest()) {
        for (int i = 0; i < neededResources_.size(); ++i) {
            // Process allocated what was needed
            allocatedResources_[i] += neededResources_[i]; 
            // Allocated Resources are no longer available
            availableResources_->at(i) -= neededResources_[i];
            // No more instances of resource needed
            neededResources_[i] = 0;
        }
    }

}

// Process finishes task; releases its allocated resources, sets maximum 
// resourcs back to 0 for each resource type, and sets state to finished
void Process::finishTask() {
    for (int i = 0; i < neededResources_.size(); ++i) {
        // Release allocated resources
        availableResources_->at(i) += allocatedResources_[i];
        allocatedResources_[i] = 0; 
        maximumResources_[i] = 0;
    }
    // Process has finishes running
    state_ = State::finished;

}

int main()
{
    ifstream dataFile; // Input stream for process-resource data
    dataFile.open("resource_allocation.txt");

    // give error message and exit if file failed to open
    if (dataFile.fail()) {
        std::cerr << "The data file could not be opened! Terminating program.\n";
        exit(1);
    }

    // Read data from text file
    // Get resource allocation data
    vector<vector<int>> allocation;
    string dataString;
    std::getline(dataFile, dataString); // gets "Allocation"; throw away
    std::getline(dataFile, dataString); // gets first line of data
    
    // Continue reading allocation data until the "Maximum" header is reached
    while (dataString != "Maximum") {
        // Row of allocation data; Corresponds to a process
        vector<int> allocationRow; 
        // Parse the data strings for the individual numbers
        for (int i = 0; i < dataString.size(); ++i) {
            if (dataString[i] == ' ') {} // Disregard whitespace
            // Otherwise, add the integer to the allocation row
            else
                allocationRow.push_back(dataString[i] - '0');
        }
        // Adds each row to allocation matrix
        allocation.push_back(allocationRow);
        getline(dataFile, dataString);

    }

    // Get maximum resource data
    vector<vector<int>> maximum;
    // Get next data string, throws away "Allocation"
    std::getline(dataFile, dataString); 
    // Continue reading allocation data until the "Available" header is reached
    while (dataString != "Available") {
        // row of maximum data; corresponds to a process
        vector<int> maximumRow;
        // Parse the data strings for individual numbers
        for (int i = 0; i < dataString.size(); ++i) {
            if (dataString[i] == ' ') {} // Ignore whitespace
            // Otherwise add number to row
            else 
                maximumRow.push_back(dataString[i] - '0');
        }
        // Add each row to maximum matrix
        maximum.push_back(maximumRow);
        std::getline(dataFile, dataString);

    }

    // Get the available resource data
    vector<int> initiallyAvailable;
    std::getline(dataFile, dataString); // gets next row of numbers
    for (int i = 0; i < dataString.size(); ++i) {
        if (dataString[i] == ' ') {} // ignore whitespace
        else
            initiallyAvailable.push_back(dataString[i] - '0');
    }
    
    const int processCount = allocation.size() - 1; // number of rows = number of processes
    vector<Process> P; // Processes in the system
    // Create and add processes to the system
    for (int i = 0; i < processCount; ++i) {
        P.push_back(Process(allocation[i], maximum[i], &initiallyAvailable));
    }
    

    // Order sequence of processes which identifies the order in which the
    // process' requests should be served to maintain safe state
    vector<string> safeSequence; 
    bool systemRunning = true; // true if at least one process is still running
    int systemSafe = 0; // 0 if none of the processes can be served 
                        // without putting the system in an unsafe state; 
                        // non-zero otherwise
    while(systemRunning) {
        // Reset system boolean; will convert to true if any processes
        // still running
        systemRunning = false;
        systemSafe = 0;
        // Add 1 to systemSafe for each safe process
        for (const auto& process : P) {
            systemSafe += process.safeToRequest();
        }
        // If none of the requests can be served safely, the system is in unsafe state
        if (systemSafe == 0) {
            cout << "The system is in an unsafe state\n";
            break;
        }
        for (int i = 0; i < processCount; ++i) {
            // Survey all unfinished processes
            if (P[i].getState() != State::finished) {
                systemRunning = true;
                // Serve process if doing so would leave system in safe state
                if (P[i].safeToRequest()) {
                    systemSafe += true; // One request can be served; temp-safe
                    P[i].requestResources(); // Request additional needed res.
                    P[i].finishTask(); // Process finished

                    // Append process name to safe sequence
                    safeSequence.push_back("P" + std::to_string(i));

                }
            }
        
        }
        
    }
    // If system is in safe state, print safe sequence
    if (systemSafe != 0) {
        cout << "The system is safe!\n";
        cout << "Safe sequence: <";
        for (int i = 0; i < safeSequence.size(); ++i) {
                cout << safeSequence[i];
                if (i != safeSequence.size()-1) cout << ", ";
        } cout << ">\n";

    }

}