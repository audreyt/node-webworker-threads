function DispatchEvents (thread)
    thread = this <<< {
        on: (e, f, q) ->
            if q = thread._on[e] then q.push f else thread._on[e] = [f]
            return thread
        once: (e, f, q) ->
            not (q = thread._on[e]) and (q = thread._on[e] = [])
            if q.once then q.once.push f else q.once = [f]
            return thread
        remove-all-listeners: (e) ->
            if arguments_.length then delete! thread._on[e] else thread._on = {}
            return thread
        dispatch-events: (event, args, q, i, len) ->
            if q = thread._on[event] => try
                i = 0
                len = q.length
                while i < len
                    q[i++].apply thread, args
                if q = q.once
                    q.once = ``undefined``
                    i = 0
                    len = q.length
                    while i < len
                        q[i++].apply thread, args
            catch
              # e = new Error e unless e instanceof Error
              # Error.captureStackTrace e
              __postError e
        _on: {}
    }
    return @dispatch-events
