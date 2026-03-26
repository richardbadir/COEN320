#ifndef AIRTRAFFICCONTROL_H
#define AIRTRAFFICCONTROL_H

#include "Aircraft.h"
#include <vector>
#include <thread>
#include <string>

// Struct to hold the plane data temporarily
struct PlaneData {
    int arrivaTime;
	int id;
    int posX, posY, posZ;
    int speedX, speedY, speedZ;
};

class AirTrafficControl {
public:
    AirTrafficControl();
    ~AirTrafficControl();

    // Reads the file and creates aircraft instances
    void readPlanesFromFile(const std::string& fileName);

    // Starts all planes (i.e., creates and joins their threads)
    void startPlanes();
    bool areAllPlanesFinished() const;

private:
    std::vector<Aircraft*> planes;  // Stores all aircraft objects
    std::vector<PlaneData> planeData;  // Stores the plane data
    bool allPlanesFinished = false;  // Flag to indicate all planes are done
};

#endif // AIRTRAFFICCONTROL_H
