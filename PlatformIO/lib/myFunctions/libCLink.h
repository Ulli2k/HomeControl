
#ifndef __libCLink_H__
#define __libCLink_H__

#include <libAtomic.h>

/******** DEFINE dependencies ******

************************************/

class libCLink {
  // LinkElement(successor1(successor2(successor...)))

  // pointer to successor element
  libCLink* link;

public:
  libCLink () : link(0) {}
  libCLink (libCLink* item) : link(item) {}

  // return pointer to successor element
  libCLink* select () const {
    libCLink* result = 0;
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
	    result = link;
	  }
    return result;
  }

  // define successor
  void select (libCLink* item) {
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
      link=item;
    }
  }

  // add item as first successor of element
  // current(successor1) --> current(item(successor1))
  void append (libCLink& item) {
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
      item.select(select()); //move successor element to define item element
      select(&item); //define successor of current element
    }
  }

  // return tail item
  libCLink* ending () const {
	libCLink* item=0;
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
      item=(libCLink*)this;
      while( item->select() != 0 ) {
        item = item->select();
      }
    }
    return item;
  }

  // remove successor and return it
  libCLink* unlink () {
    libCLink* item=0;
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
      item=select(); //get successor
      if( item!=0 ) {
        detach(); //remove successor
      }
    }
    return item;
  }

  // remove all, return successor
  libCLink* remove () {
	  libCLink* item=0;
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
      item=select();
      select(0);
    }
    return item;
  }

  // remove successor
  //current(successor1(successor2)) -> current(successor2)
  void detach () {
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
      select(select()->select());
    }
  }

  // return container instance
  libCLink* search (const libCLink* item) const {
    libCLink* result = 0;
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
      libCLink* tmp = select();
      libCLink* vor = (libCLink*)this;
      while (result == 0 && tmp != 0) {
        if (tmp == item) {
          result = vor;
        }
        vor = tmp;
        tmp = tmp->select();
      }
    }
    return result;
  }

  // remove item
  void remove (const libCLink& item) {
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
      libCLink* vor = search(&item);
      if( vor != 0 ) {
        vor->unlink();
      }
    }
  }
};

#endif
