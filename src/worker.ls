function Worker () => Threads = this; class
    (code) ->
        @t = t = Threads.create!
        @t.on \message ~> @onmessage? data: it
        @t.on \close -> t.destroy!
        @terminate = -> t.destroy!
        @add-event-listener = (event, cb) ~> @onmessage = cb
        @post-message = (msg) -> t.emit \message msg
        if typeof code is \function
            @t.eval "(#code)()"
        else
            @t.load code
