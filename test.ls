#!/usr/bin/env lsc
{ Worker } = require \webworker-threads
w = new Worker ->
    # This also works, but less elegant:
    # ``onmessage`` = (data: {max}) ->
    @onmessage = (data: {max}) ->
        :search for n from 2 to max
            for i from 2 to Math.sqrt n
                continue search unless n % i
            postMessage { result: n }
    postMessage {+done}
w.onmessage = (data: {done, result}) ->
    return setTimeout((~> @terminate!), 100ms) if done
    console.log "#result is a prime"
w.postMessage max: 10000
