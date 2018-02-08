var Threads = require('../');
var worker = new Threads.Worker(__dirname + '/nextloop_worker.js');
worker.addEventListener('close', () => {
	worker.terminate();
});