#ifndef SYSTEM_ROOTCA_H
#define SYSTEM_ROOTCA_H

/* The root certificate this node trusts for HTTPS to its own server.
 *
 * One place, because two things need it - OTA (system/ota.cpp) and enrolment (system/mqtt.cpp) -
 * and because it expires: ISRG Root X1 runs to 2035, and Let's Encrypt has newer roots. A node with
 * a stale root cannot fetch firmware or a credential, and cannot be told so remotely. Somewhere
 * findable is worth more than saving a file.
 *
 * Declared here and defined once in rootca.cpp rather than being a header constant, so the ~1.8 KB
 * is in the image once however many places include this.
 */
const char* rootCAForServer();

#endif // SYSTEM_ROOTCA_H
