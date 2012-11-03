function ThreadNextTick
    function next-tick (cb)
        thread._ntq.push cb
        return this
    function dispatch-next-ticks (l, p, err, _ntq)
        if l = (_ntq = thread._ntq).length
            p = err = 0
            try
                while true
                    _ntq[p]!
                    break unless ++p < l
            catch e
                thread._ntq = _ntq.slice ++p
                throw e
            return (thread._ntq = _ntq.slice p).length
        return 0
    thread._ntq = []
    thread.next-tick = next-tick
    self.addEventListener = (event, cb) ->
        @thread.on event, (data) -> cb {data}
    self.close = -> @thread.emit \close
    Object.defineProperty self, \onmessage do
        set: (cb) -> @addEventListener \message cb
    return dispatch-next-ticks
