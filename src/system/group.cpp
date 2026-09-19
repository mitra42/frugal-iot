/* Frugal-IoT - system_group 
 * 
 * System_Group is a collection of System_Base (which could include other System_Group) and the 
 * main purpose of this class is to allow easy looping through them.
 *
 */

#include <string.h> // for strcmp in heapReport
#include "system/group.h"
#include "system/frugal.h" // for frugal_iot.messages->outgoingCount()

/* How many messages may be waiting upstream before discovery pauses.
 *
 * Each System_Message holds several Strings and measured about 390 bytes on an ESP32-S2, so 8 is
 * roughly 3KB of queue - small enough to leave WiFi and lwIP the contiguous memory they need, and
 * no real cost in time because loop() comes round again immediately. It also keeps
 * System_Messages::sendRemote() fast: that de-duplicates by scanning the whole queue on every
 * send, so a short queue turns discovery from O(n^2) into O(n * this).
 */
#ifndef SYSTEM_DISCOVERY_QUEUE_MAX
  #define SYSTEM_DISCOVERY_QUEUE_MAX 8
#endif

#ifdef SYSTEM_GROUP_HEAP_DEBUG
  // The free TOTAL is not the interesting number. WiFi and lwIP need CONTIGUOUS internal memory
  // for their buffers - PSRAM cannot be used for DMA - so it is the largest block that decides
  // whether a packet can go out or a station can associate. A heap with 19KB free in 468-byte
  // pieces is, for them, full. Printing both after every module says which module costs what,
  // and where a large block got broken up.
  #ifdef ESP8266
    #define GROUP_LARGEST_BLOCK() ESP.getMaxFreeBlockSize()
  #else
    #define GROUP_LARGEST_BLOCK() ESP.getMaxAllocHeap()
  #endif

// forEach drives setup() but ALSO loop(), which runs continuously - printing a line per module
// per pass scrolls the one thing we are looking for off the screen in a second. So print every
// line during setup, which is a single pass and is where the memory is actually spent, and after
// that print only when the numbers have MOVED. Steady state is then silent, and a leak or a big
// transient is the only thing that shows up.
static void heapReport(const char* fnName, const char* groupId, const char* memberId) {
  static uint32_t lastFree = 0;
  static uint32_t lastLargest = 0;
  const uint32_t freeNow = ESP.getFreeHeap();
  const uint32_t largestNow = GROUP_LARGEST_BLOCK();
  // 1024 is small enough to catch a module quietly eating memory, big enough that the ordinary
  // String churn of a loop pass does not print.
  const bool moved = (labs((long)freeNow - (long)lastFree) > 1024)
                  || (labs((long)largestNow - (long)lastLargest) > 1024);
  if ((strcmp(fnName, "setup") == 0) || moved) {
    Serial.printf("heap %s %s/%s: free=%u largest=%u\n", fnName, groupId, memberId,
      (unsigned)freeNow, (unsigned)largestNow);
    lastFree = freeNow;
    lastLargest = largestNow;
  }
}
#endif

System_Group::System_Group(const char * const id, const char * const name)
: System_Base(id, name)
{}

System_Base* System_Group::add(System_Base* fb) {
  group.push_back(fb);
  return fb;
}


// Loops over the members of the group calling fn on each, printing fnName + the group's and
// member's id when SYSTEM_GROUP_DEBUG is defined.
void System_Group::forEach(const char* fnName, void (System_Base::*fn)()) {
  for (System_Base* fb: group) {
    #ifdef SYSTEM_GROUP_DEBUG
      Serial.printf("%s: %s/%s\n", fnName, id, fb->id);
    #endif
    (fb->*fn)();
    #ifdef SYSTEM_GROUP_HEAP_DEBUG
      heapReport(fnName, id, fb->id);
    #endif
  }
}

void System_Group::setup()        { forEach("setup", &System_Base::setup); }
/* Describe every member to the server, a few at a time, pausing while the queue is full.
 *
 * This used to be forEach("discover", ...), which walked the whole tree in one call and queued
 * roughly a hundred System_Message objects before loop() got a chance to drain any of them. On an
 * ESP32-S2 that took free heap from 42,620 bytes down to 1,176 and left the largest contiguous
 * free block under 1.5KB PERMANENTLY - fatal, because WiFi and lwIP need contiguous internal
 * memory and cannot use PSRAM. The captive portal could build its page and never transmit it.
 *
 * Resuming cannot be done with an index into group[], because this is recursive - a nested group
 * can stop partway through its own members, and an index in the parent cannot say that. So the
 * position is the 'discovered' flag on each System_Base. A leaf is marked by its parent, since it
 * always finishes in one call; a group marks ITSELF at the end, so it stays unmarked if it broke
 * out early and its parent then knows not to move past it.
 *
 * Re-walking and letting System_Messages::sendRemote() de-duplicate does NOT work instead: it
 * de-duplicates against messages still IN the queue, so anything already sent and erased would
 * quietly be queued a second time.
 */
void System_Group::discover() {
  bool finished = true;
  for (System_Base* fb: group) {
    if (!fb->discovered) {
      if (frugal_iot.messages->outgoingCount() >= SYSTEM_DISCOVERY_QUEUE_MAX) {
        finished = false;
        break; // Resume from this member on a later loop(), once the queue has drained
      }
      fb->discovered = true; // A leaf completes in one call; a group that does not clears this
      fb->discover();
      #ifdef SYSTEM_GROUP_HEAP_DEBUG
        heapReport("discover", id, fb->id);
      #endif
      if (!fb->discovered) {
        finished = false;
        break; // A nested group stopped partway - do not advance past it
      }
    }
  }
  discovered = finished;
}
void System_Group::prepare()      { forEach("prepare", &System_Base::prepare); }
void System_Group::recover()      { forEach("recover", &System_Base::recover); }
void System_Group::loop()         { forEach("loop", &System_Base::loop); }
void System_Group::periodically() { forEach("periodically", &System_Base::periodically); }
void System_Group::infrequently() { forEach("infrequently", &System_Base::infrequently); }

void System_Group::captiveLines(AsyncResponseStream* response)
  { for (System_Base* fb: group) { fb->captiveLines(response); } }
  
void System_Group::dispatch(System_Message &msg) {
  for (System_Base* fb: group) {
    fb->dispatch(msg);
  }
}

