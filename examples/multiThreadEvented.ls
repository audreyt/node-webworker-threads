# Minimal example for https://npmjs.org/package/webworker-threads

{ Worker } = require \webworker-threads

(new Worker ->
    fibo = (n) -> if n > 1 then fibo(n - 1) + fibo(n - 2) else 1
    self.onmessage = ({ data }) -> self.postMessage fibo data
)
    ..onmessage = ({ data }) ->
        console.log data
        @postMessage Math.ceil Math.random! * 30
    ..postMessage Math.ceil Math.random! * 30

do spin = ->
    process.stdout.write '.'
    process.nextTick spin
