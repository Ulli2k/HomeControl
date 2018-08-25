
 #include "AlarmClock.h"

// --------------------------- RTC ----------------------------------
#ifdef HAS_RTC
  RTCZero rtcZero;
  cRTC rtc;
  // void RTC_callback () { //just needed for wakeup
    //  DPRINT(".");
      // rtc.overflow();
     // rtc.debug();
      // --rtc;
      // Serial1.print(".");
      // #ifdef HAS_RTC
      // rtcZero.setAlarmEpoch(rtcZero.getEpoch() + INFO_POLL_CYCLE_TIME);
      // #endif
  // }
#endif

// --------------------------- SysClock -----------------------------
SysClock sysclock;
#if defined (__SAMD21G18A__)
  extern "C" {
  	//sysTicks will be triggered once per 1 ms
  	int sysTickHook(void) {
      if(sysclock.active()) { --sysclock; }
  	  return 0; //0: default SysTickHock Function execution
  	}
  }
#else
  //Arduino works via poll and corrects SysClock after each poll
#endif
// void SysClock_callback(void) {
//   --sysclock;
// }

// --------------------------- AlarmClock ---------------------------
void AlarmClock::cancel(Alarm& item) {
  ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
  {
    Alarm *tmp = (Alarm*) select();
    libCLink *vor = this;
    // search for the alarm to cancel
    while (tmp != 0) {
      if (tmp == &item) {
        vor->unlink();
        // raise next alarm about item ticks
        tmp = (Alarm*) item.select();
        if (tmp != 0) {
          tmp->tick += item.tick;
        }
        return;
      }
      vor = tmp;
      tmp = (Alarm*) tmp->select();
    }
    // cancel also in ready queue
    ready.remove(item);
  }
}

AlarmClock& AlarmClock::operator --() {
  ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
  {
    Alarm* alarm = (Alarm*) select();
    if (alarm != 0) {
      --alarm->tick; //reduce tick value of just the first successor enough because it's relative tick order
      while ((alarm != 0) && (alarm->tick == 0)) {
        unlink(); // remove expired alarm
        // run in interrupt
        if (alarm->async() == true) {
          alarm->trigger(*this);
          if(alarm->tickLoop) add(*alarm);
        }
        // run in application
        else {
          ready.append(*alarm); // add in class for just items with tick=0 (time is over)
        }
        alarm = (Alarm*) select();
      }
    }
  }
  return *this;
}

//add new successor, modifiy new successor.ticks to get relative ticks and add it in between (ticks order ascend(aufsteigend))
void AlarmClock::add(Alarm& item) {
  if(!item.tick && item.tickLoop)
    item.tick = item.tickLoop;

  if (item.tick > 0) {
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE )
    {
      libCLink* prev = this;
      Alarm* temp = (Alarm*) select(); //get first successor
      while ((temp != 0) && (temp->tick < item.tick)) { //move throw successors and substract successor.ticks from item (relative ticks)
        item.tick -= temp->tick;
        prev = temp;
        temp = (Alarm*) temp->select();
      }
      item.select(temp);
      prev->select(&item); //add item between successors
      if (temp != 0) {
        temp->tick -= item.tick; //correct ticks of item(successor)
      }
    }
  } else {
    ready.append(item); //add as first successor of element
  }
}

uint32_t AlarmClock::get(const Alarm& item) const {
  uint32_t aux = 0;
  Alarm* tmp = (Alarm*) select();
  while (tmp != 0) {
    aux += tmp->tick;
    if (tmp == &item) {
      return aux;
    }
    tmp = (Alarm*) tmp->select();
  }
  return 0;
}
