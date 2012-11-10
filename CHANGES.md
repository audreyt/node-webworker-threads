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
