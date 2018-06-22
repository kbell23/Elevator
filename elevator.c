#include "hw6.h"
#include <stdio.h>
#include <pthread.h>
#include <time.h>

struct Elevator{
  pthread_mutex_t lock;
  pthread_barrier_t barrier;
  int to_floor;
  int current_floor;
  int direction;
  int occupancy;
  int PASSENGERS_LEFT;
  enum {ELEVATOR_ARRIVED=1, ELEVATOR_OPEN=2, ELEVATOR_CLOSED=3} state;
} elevators[ELEVATORS];

struct Passenger{
  int elevator;
} passengers[PASSENGERS];	

int f[FLOORS];

void scheduler_init() {
    int i;
    int j;
    int k = 0;
	// magical loop
    for(i = 0; i < ELEVATORS; i++) {
        elevators[i].to_floor = -1;
        elevators[i].current_floor=0;		
		elevators[i].direction=-1;
		elevators[i].occupancy=0;
		elevators[i].state=ELEVATOR_ARRIVED;
		pthread_mutex_init(&elevators[i].lock, 0);
        pthread_barrier_init(&elevators[i].barrier, 0, 2);
    }
	// inits
	for(i = 0; i < PASSENGERS; i++){
		passengers[i].elevator = (i % ELEVATORS);
		elevators[i % ELEVATORS].PASSENGERS_LEFT++;
	}
	for(i = 0; i < FLOORS; i++){
		f[i] = 0;
	}
}

void passenger_request(int passenger, int from_floor, int to_floor, void (enter)(int, int), void(exit)(int, int))
{
    // wait for the elevator to arrive at our origin floor, then get in
    int waiting = 1;
    int riding = 1;
    int myElevator = passengers[passenger].elevator;
    f[from_floor]++;
    while(waiting) {
        pthread_barrier_wait(&elevators[myElevator].barrier);
        pthread_mutex_lock(&elevators[myElevator].lock);

        if(elevators[myElevator].current_floor == from_floor && elevators[myElevator].state == ELEVATOR_OPEN && elevators[myElevator].occupancy == 0) {
            enter(passenger, myElevator);
            elevators[myElevator].occupancy++;
            elevators[myElevator].to_floor = to_floor; //
			f[from_floor]--;
            waiting=0;
        }
        pthread_mutex_unlock(&elevators[myElevator].lock);
        pthread_barrier_wait(&elevators[myElevator].barrier);
    }
    // wait for the elevator at our destination floor, then get out
    while(riding) {
        pthread_barrier_wait(&elevators[myElevator].barrier);
        pthread_mutex_lock(&elevators[myElevator].lock);

        if(elevators[myElevator].current_floor == to_floor && elevators[myElevator].state == ELEVATOR_OPEN) {
            exit(passenger, myElevator);
            elevators[myElevator].occupancy--;
			elevators[myElevator].PASSENGERS_LEFT--;
            elevators[myElevator].to_floor = -1;
            riding=0;
        }
        pthread_mutex_unlock(&elevators[myElevator].lock);
        pthread_barrier_wait(&elevators[myElevator].barrier);
    }
}

void elevator_ready(int elevator, int at_floor, void(*move_direction)(int, int), void(*door_open)(int), void(*door_close)(int)) {
	if(elevator > ELEVATORS) return;
	if(elevators[elevator].PASSENGERS_LEFT <= 0) { return; }
    pthread_barrier_wait(&elevators[elevator].barrier);
    pthread_mutex_lock(&elevators[elevator].lock);
    if((elevators[elevator].state == ELEVATOR_ARRIVED) && ((f[at_floor] > 0)&& ((elevators[elevator].to_floor == -1)) || (elevators[elevator].to_floor == at_floor))){
            door_open(elevator);
            elevators[elevator].state = ELEVATOR_OPEN;
    }
    else if(elevators[elevator].state == ELEVATOR_OPEN) {
        door_close(elevator);
        elevators[elevator].state = ELEVATOR_CLOSED;
    }
    else {
		// reverse
        if(at_floor==0 || at_floor==FLOORS-1) {
            elevators[elevator].direction*=-1;
        }
        move_direction(elevator,elevators[elevator].direction);
        elevators[elevator].current_floor = at_floor + elevators[elevator].direction;
        elevators[elevator].state=ELEVATOR_ARRIVED;
    }
    pthread_mutex_unlock(&elevators[elevator].lock);
    pthread_barrier_wait(&elevators[elevator].barrier);
}