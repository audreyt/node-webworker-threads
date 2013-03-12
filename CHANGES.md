## 0.4.6

### Bug Fixes

* Compatibility with Node.js 0.10.

## 0.4.5

### Bug Fixes

* new Worker("filename.js") was broken on OS X. (@dfellis)

## 0.4.3

### Bug Fixes

* Fix Linux compilation issue introduced in 0.4.1. (@dfellis)

* `importScripts` now checks if the files have been read entirely,
  instead of (potentially) evaluating part of the file in case
  of filesystem failure.

## 0.4.2

### Global Worker API

* Set `onmessage = function(event) { ... }` directly now works
  as specced. (Previously it required `self.onmessage = ...`.)

## 0.4.1

### Global Worker API

* Add `importScripts` for loading on-disk files.

* Add `console.log` and `console.error` from thread.js.

## 0.4.0

* Support for Windows with Node.js 0.9.3+.

## 0.3.2

* Fix BSON building on SunOS.

## 0.3.1

* Switch to BSON instead of JSON for message serialization.

  Note that neither one supports circular structures or
  native buffer objects yet.

## 0.3.0

* Require Node.js 0.8.

## 0.2.3

* Add SunOS to supported OSs; tested on Linux.

## 0.2.2

* Allow an empty `new Worker()` constructor.

* Update API documentation in README.

## 0.2.1

* Allow any JSON-serializable structures in postMessage/onmessage.

## 0.2.0

* Initial release.
