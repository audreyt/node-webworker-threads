{Worker} = require \webworker-threads
w = new Worker ->
    fibo = (n) -> if n > 1 then fibo(n - 1) + fibo(n - 2) else 1
    self.onmessage = -> self.postMessage fibo it.data 
w.postMessage Math.ceil Math.random! * 30
w.onmessage = ->
    process.stdout.write it.data
    w.postMessage Math.ceil Math.random! * 30
do function spin
    process.stdout.write '.'
    process.nextTick spin
