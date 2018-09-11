#ifndef __ALARM_H__
#define __ALARM_H__

#include "libCLink.h"

class AlarmClock;

class Alarm: public libCLink {
protected:
  ~Alarm() { }
public:
  bool asyn;
  uint32_t tick;
  uint32_t tickLoop;

  virtual void trigger(AlarmClock&) = 0;

  Alarm(uint32_t t) :
      asyn(false), tick(t), tickLoop(0) {
  }
  void set(uint32_t t) {
    tick = t;
  }

  void async(bool value) {
    asyn = value;
  }
  bool async() const {
    return asyn;
  }
  bool active() {
    return (tick?true:false);
  }
};


#endif
