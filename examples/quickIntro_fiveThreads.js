function fibo (n) {
  return n > 1 ? fibo(n - 1) + fibo(n - 2) : 1;
}

function cb (err, data) {
  process.stdout.write(" ["+ this.id+ "]"+ data);
  this.eval('fibo(35)', cb);
}

var threads_a_gogo= require('threads_a_gogo');

threads_a_gogo.create().eval(fibo).eval('fibo(35)', cb);
threads_a_gogo.create().eval(fibo).eval('fibo(35)', cb);
threads_a_gogo.create().eval(fibo).eval('fibo(35)', cb);
threads_a_gogo.create().eval(fibo).eval('fibo(35)', cb);
threads_a_gogo.create().eval(fibo).eval('fibo(35)', cb);

(function spinForever () {
  process.stdout.write(".");
  process.nextTick(spinForever);
})();
