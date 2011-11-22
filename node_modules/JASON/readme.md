## JASON

JASON is just like JSON, but unlike JSON it can:

* serialize objects with methods
* serialize objects with cyclic references
* understand Dates, Regexps, Booleans, etc, and restore them with `.parse()` with their proper types/classes.
* understand and serialize all the JS primitives, including `undefined`
* properly recreate the holes in Arrays

JASON lets you pass objects as text between processes and/or threads.

Warning: unlike JSON, JASON is *unsafe*. You should only use it in contexts where you have strong guarantees that the strings that you pass to the JASON parser have been produced by a JASON formatter from a trusted source.

## Syntax

JASON syntax is just plain JavaScript (but not JSON). 

The `stringify` function does the clever work of generating whatever Javascript is needed to recreate the object, and the `parse` function is just a call to `eval`.

## Examples

See the `test/test01.js` file.

## API

``` javascript
var JASON = require("JASON");

str = JASON.stringify(obj);
obj = JASON.parse(str);
```

# Installation

The easiest way to install `JASON` is with NPM:

```sh
npm install JASON
```

## License

This work is licensed under the [MIT license](http://en.wikipedia.org/wiki/MIT_License).
