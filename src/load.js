(function load (p,cb) {
  return this.eval(require('fs').readFileSync(p, 'utf8'), cb);
})
