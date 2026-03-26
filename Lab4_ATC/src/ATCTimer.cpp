#include "ATCTimer.h"

// Constructor to initialize timer with seconds and milliseconds
ATCTimer::ATCTimer(uint32_t sec, uint32_t msec) {
	// Create a message channel for communication
	channel_id = ChannelCreate(0);
	// Attach to the message channel
	connection_id = ConnectAttach(0,0,channel_id,0,0);

	// Error handling if connection fails
	if(connection_id == -1){
		std::cerr << "Timer, Connect Attach error : " << errno << "\n";
	}
	
	// Initialize the signal event for the timer
	SIGEV_PULSE_INIT(&sig_event, connection_id, SIGEV_PULSE_PRIO_INHERIT, 1, 0);

	// Create the timer with real-time clock and the signal event
	if (timer_create(CLOCK_REALTIME, &sig_event, &timer_id) == -1){
		std::cerr << "Timer, Init error : " << errno << "\n";
	}
	
	// Set up the timer specifications (interval and initial expiration)
	setTimerSpecification(sec,1000000* msec); //converting ms to ns

	// Get the system's cycles per second for time calculations
	cycles_per_sec = SYSPAGE_ENTRY(qtime)->cycles_per_sec;
	

}

ATCTimer::~ATCTimer() {}

//Function to start the timer
void ATCTimer::startTimer(){
	timer_settime(timer_id, 0, &timer_spec, NULL);
}

// Function to set the timer specifications (time intervals)
void ATCTimer::setTimerSpecification(uint32_t sec, uint32_t nano){ // pure periodic timer
	// Set the initial time value for the timer
	timer_spec.it_value.tv_sec = sec;
	timer_spec.it_value.tv_nsec = nano;

	// Set the interval for periodic execution
	timer_spec.it_interval.tv_sec = sec;
	timer_spec.it_interval.tv_nsec = nano;

	// Start the timer with the updated specifications
	timer_settime(timer_id, 0, &timer_spec, NULL); //starts the timer
}

// Function to block and wait for the timer's signal
void ATCTimer::waitTimer(){
	// Receive a message (this call blocks until the timer pulses)
	int receive_message_id; //to identify source
	receive_message_id = MsgReceive(channel_id, &msg_buffer, sizeof(msg_buffer), NULL);
}

// Function to record the current time (in cycles)
void ATCTimer::tick(){
	// Record the current number of cycles (for time measurement)
	tick_cycles = ClockCycles();
}

// Function to calculate the elapsed time in milliseconds since the last tick
double ATCTimer::tock(){
	// Record the current number of cycles (for time measurement)
	tock_cycles = ClockCycles();
	// Calculate and return the elapsed time in milliseconds
	return (double)(tock_cycles - tick_cycles) / cycles_per_sec * 1000.0;
}

