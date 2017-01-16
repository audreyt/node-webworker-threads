

var T= require('../');

var MAX_THREADS = 1000;
var THREADS_PER_ITER = 5;

var threadCount = 0; /* # of threads created so far. */

/* Indicate create/destroy rate. */
var reportFreqInMs = 5e2;
var t= Date.now();
function display () {
  var e= Date.now()- t;
  var tps= (threadCount*1e3/e).toFixed(1);
  process.stdout.write('\nt (ms) -> '+ e+ ', threadCount -> '+ threadCount + ', created/destroyed-per-second -> '+ tps + '\n');
}

var displayInterval = setInterval(display, reportFreqInMs);

/* Create and destroy MAX_THREADS threads. */

(function again () {
  var i;
  for (i = 0; i < THREADS_PER_ITER; i++) {
    T.create().destroy();
  }
  threadCount += THREADS_PER_ITER;

  if (threadCount < MAX_THREADS) {
    setTimeout(again, 0);
  }
  else {
    clearInterval(displayInterval);
    display();
  }
})();
