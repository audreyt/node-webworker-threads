function load (p, cb)
    @eval (require \fs).readFileSync(p, 'utf8'), cb
