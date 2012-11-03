{ Worker } = require \webworker-threads
w = new Worker ->
    self.addEventListener \x (data: {max}) ->
        :search for n from 2 to max
            for i from 2 to Math.sqrt n
                continue search unless n % i
            self.postMessage { result: n }
w.onmessage = (data: {result}) ->
    console.log "#result is a prime"
    @terminate!
w.dispatchEvent type: \x data: max: 10000
