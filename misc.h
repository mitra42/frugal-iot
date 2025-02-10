#ifndef MISC_H
#define MISC_H

// And some useful functions used in various places - ifdef them for places they are used

#if defined(SYSTEM_TIME_WANT) || defined(CONTROL_WANT)
const String StringF(const char* format, ...);
#endif 

// TODO-125 ifdef or moe somewhere
void internal_watchdog_setup();
void internal_watchdog_loop();


#endif //MISC_H