function Worker () => Threads = this; class
    (code) ->
        @thread = t = Threads.create!
        t.on \message (args) ~> @onmessage? data: args
        t.on \error (args) ~> @onerror? args
        t.on \close -> t.destroy!
        @terminate = -> t.destroy!
        @add-event-listener = (event, cb) ~>
            if event is \message
                @onmessage = cb
            else
                t.on event, cb
        @dispatch-event = (event) -> t.emitSerialized event.type, null, event
        @post-message = (data, transferables) ->
          if not transferables?
            transferables = null
          t.emitSerialized \message, transferables, {data}
        if typeof code is \function
            t.eval "(#code)()"
        else if code?
            t.load code
