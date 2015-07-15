

var t= require('../');

var worker = new t.Worker(function() {
  this.onmessage = function(e) {
    postMessage(e.data);
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

// Test a regular array.
var a = new Uint8Array(64);
for (var i = 0; i < 64; i++) { a[i] = i; }
worker.postMessage({ array: a, test: 'regular'}, [a.buffer]);

// Test an array which is a view into a larger buffer.
var b = a.subarray(16, 32);
worker.postMessage({ array: b, test: 'subarray'});

worker.postMessage({ terminate: 1 });

