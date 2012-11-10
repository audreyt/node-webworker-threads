{ Worker } = require \webworker-threads
w = new Worker ->
    self.addEventListener \x (data: {max}) ->
        :search for n from 2 to max
            for i from 2 to Math.sqrt n
                continue search unless n % i
            self.postMessage { result: n }
    self.postMessage {+done}
w.onmessage = (data: {done, result}) ->
    return setTimeout((~> @terminate!), 100ms) if done
    console.log "#result is a prime"
w.dispatchEvent type: \x data: max: 10000
