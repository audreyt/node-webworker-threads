(function (thread) {
  'use strict';
  
  //2011-11 Proyectos Equis Ka, s.l., jorge@jorgechamorro.com
  //threads_a_gogo_events.js
  
  thread= this;

  function on (e, f, q) {
    (q= thread._on[e]) ? q.push(f) : (thread._on[e]= [f]);
    return thread;
  }

  function once (e, f, q) {
    !(q= thread._on[e]) && (q= thread._on[e]= []);
    q.once ? q.once.push(f) : (q.once= [f]);
    return thread;
  }
  
  function removeAllListeners (e) {
    arguments.length ? delete thread._on[e] : (thread._on= {});
    return thread;
  }
  
  function dispatchEvents (event, args, q, i, len) {
    if (q= thread._on[event]) {
      i= 0;
      len= q.length;
      while (i < len) {
        q[i++].apply(thread, args);
      }
      
      if (q= q.once) {
        q.once= undefined;
        i= 0;
        len= q.length;
        while (i < len) {
          q[i++].apply(thread, args);
        }
      }
    }
  }
  
  thread.on= on;
  thread._on= {};
  thread.once= once;
  thread.removeAllListeners= removeAllListeners;
  
  return dispatchEvents;
})
