{ Worker } = require \webworker-threads

for til 5 => (new Worker ->
    fibo = (n) ->
        | n > 1 => fibo(n - 1) + fibo(n - 2)
        | _     => 1
    self.onmessage = ({data}) ->
        self.postMessage fibo(data)
)
    ..onmessage = ({data}) ->
        process.stdout.write "[#{@thread.id}] #data"
        @postMessage 35
    ..postMessage 35

do spinForever = ->
    process.stdout.write \.
    process.nextTick spinForever
