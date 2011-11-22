(function spinForever () {
  process.stdout.write(".");
  process.nextTick(spinForever);
})();
