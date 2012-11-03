function Worker(){
  var Threads;
  Threads = this;
  return (function(){
    var prototype = constructor.prototype;
    function constructor(code){
      var t, this$ = this;
      this.thread = t = Threads.create();
      t.on('message', function(it){
        return typeof this$.onmessage === 'function' ? this$.onmessage({
          data: it
        }) : void 8;
      });
      t.on('close', function(){
        return t.destroy();
      });
      this.terminate = function(){
        return t.destroy();
      };
      this.addEventListener = function(event, cb){
        return this$.onmessage = cb;
      };
      this.postMessage = function(msg){
        return t.emit('message', msg);
      };
      if (typeof code === 'function') {
        t.eval("(" + code + ")()");
      } else {
        t.load(code);
      }
    }
    return constructor;
  }());
}
