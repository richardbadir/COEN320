/*
 * The cTimer class is designed to manage and operate a hardware timer,
 * particularly in a real-time using a message-passing mechanism to handle timer events.
 *
 * GOAL: manage periodic tasks that need to be executed at specific time intervals.
 * For example, we need to manage the updatePosition of our planes
 *
 * *****Timer Setup and Management*****:
 * The class creates and manages a periodic timer. This timer can be configured to expire
 * at a specific time and then repeat itself at the set interval.
 * It uses system calls and libraries (timer_create, SIGEV_PULSE_INIT, timer_settime)
 * to create and control the timer.
 *
 * The constructor (cTimer::cTimer) initializes the timer, setting up a message
 * channel and attaching a signal event for handling timer interrupts or pulses.
 * The timer is then set with an initial expiration time (sec, msec) and a periodic interval.
 *
 * *****Message Handling for Timer Events:*****
 * The timer pulses (interrupts) are communicated via message passing.
 * The class uses a message channel (ChannelCreate and MsgReceive) to receive signals
 * when the timer expires. The waitTimer function blocks until a timer pulse occurs,
 * indicating that the timer's set interval has elapsed. This could be useful in systems
 * where an application needs to wait for periodic events (e.g., sensor sampling, task scheduling).
 *
 * *****Time Measurement****:
 * The class provides a mechanism to measure elapsed time by using the system's clock cycles.
 * Uses tick() and tock() to find elapsed time
 *
 * ***Customize timer****
 * Users can customize the timer's interval by specifying seconds and milliseconds
 * (via the setTimerSpec function)
 *
 */

#ifndef ATCTIMER_H_
#define ATCTIMER_H_

#include <stdio.h>
#include <iostream>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sync.h>
#include <sys/siginfo.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <sys/syspage.h>
#include <inttypes.h>
#include <stdint.h>

class ATCTimer {
	int channel_id;  		// The ID of the message channel
	int connection_id;		// The ID for the connection to the timer

	// Structure for timer signal event
	struct sigevent sig_event;

	// Structure for timer specification (time intervals)
	struct itimerspec timer_spec;

	// Timer identifier
	timer_t timer_id;

	char msg_buffer[100];			// Buffer for receiving messages

	// Clock-related variables
	uint64_t cycles_per_sec; 			// Cycles per second, for time calculation
	uint64_t tick_cycles, tock_cycles;	// Variables to store cycle counts for time measurement
public:
	// Constructor to initialize timer with seconds and milliseconds
	ATCTimer(uint32_t,uint32_t);


	// Function to set the timer specifications (time intervals)
	void setTimerSpecification(uint32_t,uint32_t);

	// Function to block until the timer expires
	void waitTimer();

	// Function to start the timer
	void startTimer();

	// Function to record the current time for tick
	void tick();
	// Function to calculate the elapsed time since the last tick
	double tock();

	virtual ~ATCTimer(); //destructor
};

#endif /* ATCTIMER_H_ */
