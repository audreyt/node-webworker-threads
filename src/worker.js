function Worker(){
  var Threads;
  Threads = this;
  return (function(){
    var prototype = constructor.prototype;
    function constructor(code){
      var t, this$ = this;
      this.thread = t = Threads.create();
      t.on('message', function(json){
        return typeof this$.onmessage === 'function' ? this$.onmessage({
          data: JSON.parse(json)
        }) : void 8;
      });
      t.on('close', function(){
        return t.destroy();
      });
      this.terminate = function(){
        return t.destroy();
      };
      this.addEventListener = function(event, cb){
        if (event === 'message') {
          return this$.onmessage = cb;
        } else {
          return t.on(event, cb);
        }
      };
      this.dispatchEvent = function(event){
        return t.emit(event.type, JSON.stringify(event));
      };
      this.postMessage = function(data){
        return t.emit('message', JSON.stringify({
          data: data
        }));
      };
      if (typeof code === 'function') {
        t.eval("(" + code + ")()");
      } else if (code != null) {
        t.load(code);
      }
    }
    return constructor;
  }());
}
