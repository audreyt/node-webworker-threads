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
    self.add-event-listener = (event, cb) ->
        @thread.on event, (json) -> cb JSON.parse json
    self.close = -> @thread.emit \close
    Object.define-property self, \onmessage do
        set: (cb) -> @add-event-listener \message cb
    return dispatch-next-ticks
