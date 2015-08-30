// run with node --expose_gc.

var t= require('../');

var worker = new t.Worker(function() {
  this.onmessage = function(e) {
    if (e.data.array) {
      postMessage({ array: e.data.array, test: e.data.test }, [e.data.array.buffer]);
    } else {
      postMessage(e.data);
    }
  }
});

var errors = [];
worker.onmessage = function(event) {
  if (event.data.terminate) {
    for (var i = 0; i < errors.length; i++) {
      console.log('ERROR ' + errors[i]);
    }
    if (errors.length == 0) {
      console.log("All tests OK");
    }
    worker.terminate();
    return;
  }
  var array = event.data.array;
  if (!(array instanceof Uint8Array)) {
    errors.push('ERROR: event.data.array must be Uint8Array');
  }
  if (event.data.test == 'regular') {
    for (var i = 0; i < array.length; i++) {
      if (array[i] != i) {
        errors.push('ERROR: incorrect data at ' + i);
        return;
      }
    }
    console.log('regular deserialization OK');
  }
  if (event.data.test == 'subarray') {
    for (var i = 0; i < array.length; i++) {
      if (array[i] != i + 16) {
        errors.push('ERROR: incorrect data at ' + i);
        return;
      }
    }
    console.log('sub-array deserialization OK');
  }
};

worker.onerror = function (event) {
  console.log('error', event);
}

// Test a regular array.
var a = new Uint8Array(64);
for (var i = 0; i < 64; i++) { a[i] = i; }
worker.postMessage({ array: a, test: 'regular'});

// Test an array which is a view into a larger buffer.
var b = a.subarray(16, 32);
worker.postMessage({ array: b, test: 'subarray'});

// Test transferable serialization
var t = new Uint8Array(b, 0, 64);
for (var i = 0; i < 64; i++) { t[i] = i; }
worker.postMessage({ array: t, test: 'regular'}, [t.buffer]);
if (t[0] == 0) {
  console.log("ERROR: array was still accessible after transfer");
}

worker.postMessage({ terminate: 1 });
gc();
