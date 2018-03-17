// Generated by LiveScript 1.3.1
(function(){
  var Worker, w, slice$ = [].slice;
  Worker = require('./').Worker;
  w = new Worker(function(){
    importScripts('test-imported-script.js');
    return this.onmessage = foo;
  });
  w.onmessage = function(arg$){
    var result;
    result = arg$.data.result;
    return console.log(result + " is a prime");
  };
  w.onerror = function(arg$){
    var message, this$ = this;
    message = arg$.message;
    console.log("Caught:", message);
    return (typeof setImmediate != 'undefined' && setImmediate !== null
      ? setImmediate
      : partialize$.apply(this, [setTimeout, [void 8, 100], [0]]))(function(){
      return this$.terminate();
    });
  };
  w.postMessage({
    max: 100
  });
  function partialize$(f, args, where){
    var context = this;
    return function(){
      var params = slice$.call(arguments), i,
          len = params.length, wlen = where.length,
          ta = args ? args.concat() : [], tw = where ? where.concat() : [];
      for(i = 0; i < len; ++i) { ta[tw[0]] = params[i]; tw.shift(); }
      return len < wlen && len ?
        partialize$.apply(context, [f, ta, tw]) : f.apply(context, ta);
    };
  }
}).call(this);
