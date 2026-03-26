#ifndef AIRCRAFT_H_
#define AIRCRAFT_H_

#include <iostream>
#include <sys/dispatch.h>
#include <thread>
#include "Msg_structs.h"


typedef struct {
int lower_x_boundary;
int upper_x_boundary;
int lower_y_boundary;
int upper_y_boundary;
int lower_z_boundary;
int upper_z_boundary;
} airspace_struct;

class Aircraft {
public:
	// Constructor
    Aircraft(int id, double x, double y, double z, double sx, double sy, double sz, int Arrivalt);
    ~Aircraft();

    //print initial aircraft info
    void printInitialAircraftData() const;

    // Print plane details - for debugging
    void printAircraft() const;

    // Starts the position update thread
    int updatePosition();

    //get Aircraft's arrival time
    int getArrivalTime();
    int getID();

    //change heading by changing speed in the xyz direction
    void changeHeading(double Vx, double Vy, double Vz);



    pthread_t thread_id;   // Thread for updating position

private:
    int id;                     // Plane ID
    double posX, posY, posZ;    // Position
    double speedX, speedY, speedZ; // Speed
    int arrivalTime;            // Time of Arrival
    int message_id;				//to identify who sends the service
    bool inAirspace;
    int Radar_id;
    airspace_struct airspace;
    //Message creation
    Message createEnterAirspaceMessage(int planeID);
    Message createExitAirspaceMessage(int planeID);
    Message createPositionUpdateMessage(int planeID, const msg_plane_info& info);
};

#endif /* AIRCRAFT_H_ */
