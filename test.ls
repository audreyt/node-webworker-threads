#!/usr/bin/env lsc
{ Worker } = (require './')
w = new Worker ->
  # This also works, but less elegant:
  # ``onmessage`` = (data: {max}) ->
  importScripts 'test-import-script.js';
  @onmessage = foo
w.onmessage = (data: {result}) ->
  console.log "#result is a prime"
w.onerror = ({message}) ->
  console.log "Caught:", message
  <~ (setImmediate ? setTimeout _, 100ms)
  @terminate!
w.postMessage max: 100
