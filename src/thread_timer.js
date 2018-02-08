function ThreadSetImmediate(){
  function setImmediate(cb){
    thread._nlp.push(cb);
    return this;
  }
  function clearImmediate(cb){
    var i = thread._nlp.indexOf(cb);
    if (i >= 0) thread._nlp.splice(i, 1);
    return this;
  }
  function dispatchSetImmediate(l, p, err, _nlp){
    _nlp = thread._nlp.map(i => i);
    thread._nlp.splice(0, thread._nlp.length);
    if (l = _nlp.length) {
      err = 0;
      try {
        for (p = 0; p < l; p ++) {
          _nlp[p]();
        }
      } catch (e) {
        _nlp = _nlp.slice(++p);
        _nlp.map(i => thread._nlp.push(i));
        throw e;
      }
      return _nlp.length;
    }
    return 0;
  }
  thread._nlp = [];
  thread.setImmediate = setImmediate;
  thread.clearImmediate = clearImmediate;

  thread.AllTimerTag = Symbol('AllTimerTag');
  var timerPool = [];
  const getStamp = () => new Date().getTime();
  const invokeTask = () => {
    var pending = [], now = getStamp();
    for (var i = 0, l = timerPool.length; i < l; i ++) {
      let task = timerPool[i];
      if (task.expire <= now) {
        pending.push(task);
        if (!task.keep) {
          timerPool.splice(i, 1);
          l --;
          i --;
        }
      }
      else break;
    }
    if (timerPool.length > 0) setImmediate(invokeTask);
    pending.map(task => {
      var t = 0;
      if (task.tag === 2 && task.delay !== 2) {
        t = task.delay === 0 ? now : getStamp();
      }
      task.func();
      if (task.tag === 2 && task.delay == 2) {
        t = getStamp();
      }
      if (task.tag === 2) {
        task.expire = t + task.duration;
      }
    });
  };
  const dispatchTask = (task) => {
    timerPool.push(task);
    timerPool.sort((ta, tb) => ta.expire - tb.expire);
    setImmediate(invokeTask);
  };
  const removeTask = (tag, func) => {
    var index = -1;
    timerPool.some((task, i) => {
      if (task.tag === tag && task.func === func) {
        index = i;
        return true;
      }
    });
    if (index < 0) return;
    timerPool.splice(index, 1);
  };
  const removeAllTask = (tag) => {
    var l = timerPool.length, i;
    for (i = l - 1; i >= 0; i --) {
      let task = timerPool[i];
      if (task.tag === tag) {
        timerPool.splice(i, 1);
      }
    }
  };
  thread.setTimeout = (cb, delay) => {
    delay = (delay * 1) || 0;
    var data = {
      tag: 1,
      func: cb,
      expire: getStamp() + delay,
      duration: delay,
      keep: false,
      delay: 0
    };
    dispatchTask(data);
  };
  thread.clearTimeout = cb => {
    if (cb === thread.AllTimerTag) removeAllTask(1);
    else removeTask(1, cb);
  };
  thread.setInterval = (cb, delay, delayMode) => {
    delay = (delay * 1) || 0;
    var data = {
      tag: 2,
      func: cb,
      expire: getStamp() + delay,
      duration: delay,
      keep: true,
      delay: delayMode || 0
    };
    dispatchTask(data);
  };
  thread.clearInterval = cb => {
    if (cb === thread.AllTimerTag) removeAllTask(2);
    else removeTask(2, cb);
  };

  return dispatchSetImmediate;
}
