#include "Radar.h"
#include <sys/dispatch.h>


Radar::Radar(uint64_t& tick_counter) : tick_counter_ref(tick_counter), activeBufferIndex(0), timer(1,0), stopThreads(false) {
	initSharedMemory();
	clearSharedMemory(); //For future Use
	// Start threads for listening to airspace events
    Arrival_Departure = std::thread(&Radar::ListenAirspaceArrivalAndDeparture, this);
    UpdatePosition = std::thread(&Radar::ListenUpdatePosition, this);
    Radar_channel = NULL;
}

Radar::~Radar() {
    shutdown();

    if (sharedMemPtr != nullptr && sharedMemPtr != MAP_FAILED) {
        munmap(sharedMemPtr, SHARED_MEMORY_SIZE);
        sharedMemPtr = nullptr;
    }

    if (shm_fd != -1) {
        close(shm_fd);
        shm_fd = -1;
    }
}

void Radar::shutdown() {
    // Set stop flag and wait for threads to complete
    stopThreads.store(true);

    // If the channel exists, close it properly
    if (Radar_channel) {
        name_detach(Radar_channel, 0);
    }

    if (Arrival_Departure.joinable()) {
        Arrival_Departure.join();
    }
    if (UpdatePosition.joinable()) {
        UpdatePosition.join();
    }
}


// Method to get the current active buffer
std::vector<msg_plane_info>& Radar::getActiveBuffer() {
    return planesInAirspaceData[activeBufferIndex];
}
//Coen320_Lab (Task0): Create channel to be reachable by radar that wants to poll the Airplane
//Radar Channel name should contain your group name
//To choose the channel with concatenating your group name with "Radar"
//Note: It is critical to not interfere with other groups
void Radar::ListenAirspaceArrivalAndDeparture() {
	Radar_channel = name_attach(NULL, "Radar_Richard_Yahia", 0);
	if (Radar_channel == NULL) {
		std::cerr << "Failed to create channel for Radar" << std::endl;
		exit(EXIT_FAILURE);
	}
	// Simulated listening for aircraft arrivals and departures
    while (!stopThreads.load()) {
        // Replace with IPC
        Message msg;
        int rcvid = MsgReceive(Radar_channel->chid, &msg, sizeof(msg), nullptr); // Replace with actual channel ID
        if (rcvid == -1) {
        	// Silently skip if MsgReceive fails, but no crash happens
        	// std::cerr << "Error receiving airspace message:" << strerror(errno) << std::endl;
        	continue;
        }

        // Reply back to the client
        int msg_ret = msg.planeID;
        MsgReply(rcvid, 0, &msg_ret, sizeof(msg_ret)); // Send plane's ID back to airplane

        switch (msg.type) {
        case MessageType::ENTER_AIRSPACE:
            addPlaneToAirspace(msg);
            break;
        case MessageType::EXIT_AIRSPACE:
            removePlaneFromAirspace(msg.planeID);
            break;
        default:
        	//All other messages dropped
            //std::cerr << "Unknown airspace message type" << std::endl;
        	break;
        }

    }
}

void Radar::ListenUpdatePosition() {

    while (!stopThreads.load()) {
    	timer.waitTimer(); // Wait for the next timer interval before polling again
    	// Only poll airspace if there are planes
        if (!planesInAirspace.empty()) {
            pollAirspace();  // Call pollAirspace() to gather position data
            writeToSharedMemory();  // Write active buffer to shared memory //For future Use
            wasAirspaceEmpty = false;
        } else if (!wasAirspaceEmpty){
        	// Only write empty buffer once after transition to empty
        	writeToSharedMemory();  // Write to shared mem when all planes have left the airspace //For future Use
        	wasAirspaceEmpty = true;  // Set flag to indicate airspace is empty
        } else{
        	//std::cout << "Airspace is empty\n";
        }

    }
}

void Radar::pollAirspace(){

	airspaceMutex.lock();
	// Make a copy of the current planes in airspace to avoid modification during iteration
	std::unordered_set<int> planesToPoll = planesInAirspace;
	airspaceMutex.unlock();

	int inactiveBufferIndex = (activeBufferIndex + 1) % 2;
	std::vector<msg_plane_info>& inactiveBuffer = planesInAirspaceData[inactiveBufferIndex];
	inactiveBuffer.clear();


	//make channel to aircraft
	for (int planeID: planesToPoll){

		airspaceMutex.lock();
		bool isPlaneInAirspace = planesInAirspace.find(planeID) != planesInAirspace.end();
		airspaceMutex.unlock();
		if (isPlaneInAirspace){
			try {
			// Confirm that the plane is still in airspace
				msg_plane_info plane_info = getAircraftData(planeID);
				inactiveBuffer.emplace_back(plane_info);
			} catch (const std::exception& e) {
				// if error to process plane get next id and exception description
				//std::cerr << "Radar: Failed to get plane data " << planeID << ": " << e.what() << "\n";
				continue;
			}
		}


		{
			std::lock_guard<std::mutex> lock(bufferSwitchMutex);
		    activeBufferIndex = inactiveBufferIndex;
		}
	}
}

msg_plane_info Radar::getAircraftData(int id) {
	//Coen320_Lab (Task0): You need to correct the channel name
	//It is your group name + plane id

	std::string id_str = "Richard_Yahia"+std::to_string(id);  // Convert integer id to string
	const char* ID = id_str.c_str();         // Convert string to const char*
	int plane_channel = name_open(ID, 0);

	if (plane_channel == -1) {
		throw std::runtime_error("Radar: Error occurred while attaching to channel");
	}

	// Prepare a message to request position data
	Message requestMsg;
	requestMsg.type = MessageType::REQUEST_POSITION;
	requestMsg.planeID = id;
	requestMsg.data = NULL;

	// Structure to hold the received position data
	Message receiveMessage;

	// Send the position request to the aircraft and receive the response
	if (MsgSend(plane_channel, &requestMsg, sizeof(requestMsg), &receiveMessage, sizeof(receiveMessage)) == -1) {
		name_close(plane_channel);
		throw std::runtime_error("Radar: Error occurred while sending request message to aircraft");
	}

	msg_plane_info received_info = *static_cast<msg_plane_info*>(receiveMessage.data);

	// Close the communication channel with the aircraft
	name_close(plane_channel);

	return received_info;
}

void Radar::addPlaneToAirspace(Message msg) {
	std::lock_guard<std::mutex> lock(airspaceMutex);
	int plane_data = msg.planeID;
    planesInAirspace.insert(plane_data);
    std::cout << "Plane " << msg.planeID << " added to airspace" << std::endl;
}

void Radar::removePlaneFromAirspace(int planeID) {
	std::lock_guard<std::mutex> lock(airspaceMutex);
	planesInAirspace.erase(planeID);  // Directly remove the integer from the list
	std::cout << "Plane " << planeID << " removed from airspace" << std::endl;
}


void Radar::writeToSharedMemory() {
    // Protect buffer switching / access to shared buffers
    std::lock_guard<std::mutex> lock(bufferSwitchMutex);

    // Get the active buffer based on the current active index
    std::vector<msg_plane_info>& activeBuffer = getActiveBuffer();

    // Get the current timestamp
    sharedMemPtr->timestamp = tick_counter_ref;

    // If active buffer is empty, try the inactive buffer
    if (activeBuffer.empty()) {
        std::vector<msg_plane_info>& inactiveBuffer =
            planesInAirspaceData[(activeBufferIndex + 1) % 2];

        if (!inactiveBuffer.empty()) {
            sharedMemPtr->is_empty.store(false);
            sharedMemPtr->count = static_cast<int>(inactiveBuffer.size());

            std::memcpy(sharedMemPtr->plane_data,
                        inactiveBuffer.data(),
                        inactiveBuffer.size() * sizeof(msg_plane_info));

            inactiveBuffer.clear();
        } else {
            sharedMemPtr->is_empty.store(true);
            sharedMemPtr->count = 0;
        }
    } else {
        sharedMemPtr->is_empty.store(false);
        sharedMemPtr->count = static_cast<int>(activeBuffer.size());

        std::memcpy(sharedMemPtr->plane_data,
                    activeBuffer.data(),
                    activeBuffer.size() * sizeof(msg_plane_info));

        activeBuffer.clear();
    }

 //might have to do some cleanup here
}

void Radar::clearSharedMemory() {
    std::lock_guard<std::mutex> lock(bufferSwitchMutex);

    if (sharedMemPtr == nullptr) {
        return;
    }

    std::memset(sharedMemPtr->plane_data, 0, sizeof(sharedMemPtr->plane_data));
    sharedMemPtr->count = 0;
    sharedMemPtr->timestamp = 0;
    sharedMemPtr->is_empty.store(true);
}
void Radar::initSharedMemory() {
    // Create or open shared memory object
    shm_fd = shm_open("/radar_shm", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    // Set size of shared memory
    if (ftruncate(shm_fd, SHARED_MEMORY_SIZE) == -1) {
        perror("ftruncate failed");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    // Map shared memory into process address space
    sharedMemPtr = static_cast<SharedMemory*>(
        mmap(nullptr,
             SHARED_MEMORY_SIZE,
             PROT_READ | PROT_WRITE,
             MAP_SHARED,
             shm_fd,
             0)
    );

    if (sharedMemPtr == MAP_FAILED) {
        perror("mmap failed");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    // Initialize memory
    std::memset(sharedMemPtr, 0, SHARED_MEMORY_SIZE);
    sharedMemPtr->is_empty.store(true);
}
