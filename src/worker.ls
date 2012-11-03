function Worker () => Threads = this; class
    (code) ->
        @thread = t = Threads.create!
        t.on \message (json) ~> @onmessage? data: JSON.parse json
        t.on \close -> t.destroy!
        @terminate = -> t.destroy!
        @add-event-listener = (event, cb) ~>
            if event is \message
                @onmessage = cb
            else
                t.on event, cb
        @dispatch-event = (event) -> t.emit event.type, JSON.stringify event
        @post-message = (data) -> t.emit \message JSON.stringify {data}
        if typeof code is \function
            t.eval "(#code)()"
        else if code?
            t.load code
