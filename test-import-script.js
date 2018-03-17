function foo (arg$){
  var max, i$, n, j$, to$, i;
  max = arg$.data.max;
  search: for (i$ = 2; i$ <= max; ++i$) {
    n = i$;
    for (j$ = 2, to$ = Math.sqrt(n); j$ <= to$; ++j$) {
      i = j$;
      if (!(n % i)) {
        continue search;
      }
    }
    postMessage({
      result: n
    });
  }
  throw 'done';
};
