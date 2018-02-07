#!/usr/bin/env bash

set -e

OUTPUT="./run-all-tests-output.log"

node ./test-package.js > ${OUTPUT}

for filename in ./test/*.js; do
    echo ${filename}
    node "${filename}" >> ${OUTPUT}
done

