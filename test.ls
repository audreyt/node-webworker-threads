{ Worker } = require \webworker-threads
w = new Worker ->
    self.onmessage = (data: max) ->
        :search for n from 2 to max
            for i from 2 to Math.sqrt n
                continue search unless n % i
            self.postMessage n
w.onmessage = (data: result) ->
    console.log "#result is a prime"
    @terminate!
w.postMessage 10000
