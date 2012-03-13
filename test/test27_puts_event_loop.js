var threads_a_gogo= require('threads_a_gogo');
var pool= threads_a_gogo.createPool(2);

var i= 0;
pool.on('again', function onAgain () {
  this.eval('ƒ()');
});

function ƒ () {
  puts(" ["+ thread.id+ "]"+ (++i));
  thread.emit('again', 0);
}

pool.all.eval(ƒ).all.eval('i=0').all.eval('ƒ()').all.eval('ƒ()').all.eval('ƒ()');
