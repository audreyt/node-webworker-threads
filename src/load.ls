function add-event-listener (event, cb)
    @thread.on event, cb
function close ()
    @thread.emit \close
function importScripts ()
    pos = -1
    if @thread.__filename
        pos = @thread.__filename.lastIndexOf '/'
        if pos == -1
            pos = @thread.__filename.lastIndexOf '\\'
    for p in arguments
        if pos >= 0 && !/^(?:[a-z]+:)?\/\//i.test p
            self.console.log("  resolving "+p+" against "+@thread.__filename.substring(0, pos + 1)+", cwd "+@thread.__cwd) #//DEBUG
            if p[0] == '!'
                escAbsolute = p[1] != '!'
                p = p.substring 1
                self.console.log("  ... removed escape character for absolute path: "+p) #//DEBUG
            if !escAbsolute && (!@thread.__cwd || p.indexOf(@thread.__cwd) != 0)
                p = @thread.__filename.substring(0, pos + 1) + p
            else #//DEBUG
                self.console.log("  ... do not resolve absolute "+p) #//DEBUG
        self.eval native_fs_.readFileSync(p, \utf8)
onmessage = null
thread.on \message (args) ~> onmessage? args
