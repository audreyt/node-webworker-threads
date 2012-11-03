function Worker(){
  var Threads;
  Threads = this;
  return (function(){
    var prototype = constructor.prototype;
    function constructor(code){
      var t, this$ = this;
      this.t = t = Threads.create();
      this.t.on('message', function(it){
        return typeof this$.onmessage === 'function' ? this$.onmessage({
          data: it
        }) : void 8;
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
        this.t.eval("(" + code + ")()");
      } else {
        this.t.load(code);
      }
    }
    return constructor;
  }());
}
