#!/usr/bin/env bash

OUTPUT="./run-all-tests-output.log"

node ./test-package.js > ${OUTPUT}

_term() {
  echo "Caught SIGTERM signal!"
  kill -TERM "$child" 2>/dev/null
}

trap _term SIGTERM

for filename in ./test/*.js; do
    timeout 5s node "${filename}" >> ${OUTPUT} &

    child=$!
    wait "${child}"

    if [ $? = 0 ]; then
        echo ${filename} "passed!"
    else
        echo ${filename} "failed!"
    fi
done
