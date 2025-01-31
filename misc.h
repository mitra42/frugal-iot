#ifndef MISC_H
#define MISC_H

// And some useful functions used in various places - ifdef them for places they are used

#if defined(SYSTEM_TIME_WANT) || defined(SYSTEM_FS_WANT)
const String StringF(const char* format, ...);
#endif 


#endif //MISC_H