#ifndef RADAR_H
#define RADAR_H

#include <atomic>  // Include to use atomic flag
#include <iostream>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <thread>
#include <string>
#include <vector>
#include <sys/mman.h>   // for shared memory
#include <fcntl.h>      // for O_CREAT, O_RDWR
#include <sys/stat.h>   // for mode constants
#include <unistd.h>     // for ftruncate, mmap
#include <cstring>      // for memset
#include "Aircraft.h"
#include "Msg_structs.h"
#include "ATCTimer.h"


// Shared memory size
#define SHARED_MEMORY_SIZE sizeof(SharedMemory)  // Update this based on the size of your buffer

class Radar {
public:
	Radar(uint64_t& tick_counter);
    ~Radar();

    void ListenAirspaceArrivalAndDeparture();
    void ListenUpdatePosition();

    // Shared memory write method
    void writeToSharedMemory();
    void clearSharedMemory();


private:

    uint64_t& tick_counter_ref;  // Store tick_counter as a reference
    std::unordered_set<int> planesInAirspace ;

    std::thread Arrival_Departure;
    std::thread UpdatePosition;

    void addPlaneToAirspace(Message msg);
    void removePlaneFromAirspace(int ID);
    void pollAirspace();
    msg_plane_info getAircraftData(int id);

    name_attach_t* Radar_channel;

    std::mutex airspaceMutex;
    std::mutex bufferSwitchMutex;


    std::vector<msg_plane_info>& getActiveBuffer();

    std::vector<msg_plane_info> planesInAirspaceData[2];
    std::atomic<int> activeBufferIndex; // Index of the active buffer

    ATCTimer timer;

    // Shared memory pointer
    SharedMemory* sharedMemPtr;  // Update pointer type to match the structure
    bool wasAirspaceEmpty = true;  // Track if airspace was empty last time
    int shm_fd = -1;
    std::atomic<bool> stopThreads;

    void shutdown();




};

#endif /* RADAR_H_ */
