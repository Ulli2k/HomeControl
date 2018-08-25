#ifndef __ALARMCLOCK_H__
#define __ALARMCLOCK_H__

#include <Alarm.h>

/******** DEFINE dependencies ******
  HAS_RTC: activates the RTCZero Class for ATSAMD21
************************************/

#if defined (__SAMD21G18A__)
  #define HAS_RTC
#endif

//!! Ticks 2 Second must be the same for RTC and SysClock!! --> 1 tick = 1 ms
#define TICKS_PER_SECOND 1000UL //ticks per second for SAMD21 SysTickHook [ms], same as Atmeaga32

#define seconds2ticks(tm) ( tm * TICKS_PER_SECOND )
#define ticks2seconds(tm) ( tm / TICKS_PER_SECOND )

#define decis2ticks(tm) ( tm * TICKS_PER_SECOND / 10 )
#define ticks2decis(tm) ( tm * 10UL / TICKS_PER_SECOND )

#define centis2ticks(tm)  ( tm * TICKS_PER_SECOND / 100 )
#define ticks2centis(tm)  ( tm * 100UL / TICKS_PER_SECOND )

#define millis2ticks(tm) ( tm * TICKS_PER_SECOND / 1000 )
#define ticks2millis(tm) ( tm * 1000UL / TICKS_PER_SECOND )

// --------------------------- AlarmClock ---------------------------
class AlarmClock: protected libCLink {

  libCLink ready; //define empty element just for items which has tick=0

public:

  void cancel(Alarm& item);

  AlarmClock& operator --();

  bool isready () const {
    return ready.select() != 0; //successor available
  }

  bool runready() {
    bool worked = false;
    Alarm* a;
    while ((a = (Alarm*) ready.unlink()) != 0) { //return successor if available and remove it
      a->trigger(*this);
      if(a->tickLoop) add(*a);
      worked = true;
    }
    return worked;
  }

  void add(Alarm& item);

  uint32_t get(const Alarm& item) const;

  uint32_t next () const {
    Alarm* n = (Alarm*)select();
    return n != 0 ? n->tick : 0;
  }

  Alarm* first () const {
    return (Alarm*)select();
  }

  // correct the alarms after sleep
  void correct (uint32_t ticks) {
    if(!ticks) return;
    ticks--;
    Alarm* n = first();
    if( n != 0 ) {
      uint32_t nextticks = n->tick-1;
      n->tick -= nextticks < ticks ? nextticks : ticks;
    }
    --(*this);
  }
};

// --------------------------- SysClock -----------------------------

// extern void SysClock_callback(void);
#if defined (__SAMD21G18A__)
  class SysClock : public AlarmClock {
  private:
    bool enabled;
  public:
    SysClock() : enabled(false)   { }
    void initialize()         { enable(); }
    bool active()             { return enabled; }
    void enable ()            { enabled=true; }
    void disable ()           { enabled=false; }
  };

#else //ATMEGA, SysClock realised via polling (millis-last_millis)

  #include <myBaseTiming.h>
  class SysClock : public AlarmClock, public myTiming {

  private:
    uint32_t last_millis;
    bool enabled;
  public:
    SysClock() : enabled(false), last_millis(0)   { }
    void initialize()         { last_millis = millis(); enable(); }
    bool active()             { return enabled; }
    void enable ()            { enabled=true; }
    void disable ()           { enabled=false; }

    bool runready() {

      if(!enabled) return false;
      uint32_t ms = millis();

      //copied from millis_since
      if(last_millis <= ms) {
    		ms =  ms - last_millis;
    	} else { //overflow
    		ms = ms + (0xFFFFFFFF - last_millis);
    	}

      correct(millis2ticks(ms));
      last_millis+=ms;
      return AlarmClock::runready();
    }
  };
#endif

extern SysClock sysclock;

// --------------------------- RTC ----------------------------------
// Is used for keep tracking time during Sleep Mode and for Waking Up in Sleep Mode by using setAlarm
// Implemented just for SAMD21

#ifdef HAS_RTC
  #include <RTCZero.h>
  extern RTCZero rtcZero;

  extern void RTC_callback(void);

  class cRTC : public AlarmClock {

  public:

    cRTC () {}

    void initialize () {
      rtcZero.begin();
      // Use RTC as a second timer instead of calendar
      rtcZero.setEpoch(0);
      enable();
    }

    void disable () {
      // rtcZero.detachInterrupt();
    }

    void enable () {
      // rtcZero.attachInterrupt(RTC_callback); //Interrupt not needed just for WakeUp Function
    }

    void setAlarm(uint32_t ticks) {
      uint32_t t = ticks2seconds(ticks);
      if(!t) t++; //round up
      // Serial1.print("Alarm: ");Serial1.println(ticks);
      rtcZero.setAlarmEpoch(rtcZero.getEpoch() + t);
      rtcZero.enableAlarm(rtcZero.MATCH_YYMMDDHHMMSS);
    }

    uint32_t getCounter (bool resetovrflow) {
      // if( resetovrflow == true ) {
      //   ovrfl = 0;
      // }
      return rtcZero.getEpoch() * seconds2ticks(1);
      // return 0;
    }

    // void debug () {
    //   if( select() != 0 ) {
    //     Serial1.print((uint16_t)((Alarm*)select())->tick);
    //     Serial1.print(" ");
    //   }
    // }
  };
  extern cRTC rtc;

#else //use millis() in poll function (runready)

// #include <myBaseTiming.h>
// class cRTC : public AlarmClock, public myTiming {
//
//   private:
//     uint32_t last_millis;
//   public:
//     cRTC () {}
//     void initialize () {
//       last_millis = millis();
//     }
//     void disable () {}
//     void enable () {}
//
//     #ifdef HAS_INFO_POLL
//     bool runready() {
//       uint32_t ms = millis();
//
//       //copied from millis_since
//       if(last_millis <= ms) {
//     		ms =  ms - last_millis;
//     	} else { //overflow
//     		ms = ms + (0xFFFFFFFF - last_millis);
//     	}
//
//       if(ms >= INFO_POLL_CYCLE_TIME*1000UL) {
//         RTC_callback();
//         last_millis=millis();
//       }
//       return AlarmClock::runready();
//     }
//     #endif
// };
#endif

#endif
