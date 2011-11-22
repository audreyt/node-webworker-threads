(function () {
  'use strict';
  
  //2011-11 Proyectos Equis Ka, s.l., jorge@jorgechamorro.com
  //threads_a_gogo_thread_nextTicks.js
  
  function nextTick (cb) {
    thread._ntq.push(cb);
    return this;
  }

  function dispatchNextTicks (l, p, err, _ntq) {
    if (l= (_ntq= thread._ntq).length) {
      p= err= 0;

      try {
        do {
          _ntq[p]();
        } while (++p < l);
      }
      catch (e) {
        thread._ntq= _ntq.slice(++p);
        throw e;
      }

      return (thread._ntq= _ntq.slice(p)).length;
    }
    return 0;
  }

  thread._ntq= [];
  thread.nextTick= nextTick;
  return dispatchNextTicks;
})()
