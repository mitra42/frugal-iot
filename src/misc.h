#ifndef MISC_H
#define MISC_H

// And some useful functions used in various places - ifdef them for places they are used

const String StringF(const char* format, ...);
const char* lprintf(size_t buffer_size, const char* format, ...);

#endif //MISC_H