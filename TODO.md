* Worker API
    * `setTimeout` / `clearTimeout` / `setInterval` / `clearInterval`
        Forwarding to the default implementation (in NativeModule TimerWrap).
    * `onerror` handler
        Catch runtime errors; also addEventListener 'error'.
    * `dispatchEvent`?
