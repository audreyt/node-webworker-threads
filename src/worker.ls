function Worker () => Threads = this; class
    (code) ->
        @thread = t = Threads.create!
        t.on \message (json) ~> @onmessage? data: JSON.parse json
        t.on \close -> t.destroy!
        @terminate = -> t.destroy!
        @add-event-listener = (event, cb) ~> @onmessage = cb
        @post-message = (msg) -> t.emit \message JSON.stringify msg
        if typeof code is \function
            t.eval "(#code)()"
        else if code?
            t.load code
