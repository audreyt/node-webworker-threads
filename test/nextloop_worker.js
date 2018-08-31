console.log('Welcome To Kosmos!!!');

var go = true;
var time = 0;
var limit = 1;

const ticker = () => {
	console.log('tick');
	if (time < limit) {
		thread.nextTick(ticker);
		time ++;
	}
	else {
		time = 0;
		go = true;
	}
};

const looper = () => {
	console.log('loop');
	thread.setImmediate(looper);
	if (go) {
		go = false;
		thread.nextTick(ticker);
	}
};

var test = () => {
	console.log('>>>>>>>>   FUCK!!!');
};
try {
	// thread.setImmediate(looper);
	thread.setInterval(test, 100);
	thread.setTimeout(() => {
		thread.clearInterval(test);
		thread.emit('close');
	}, 100);
}
catch (err) {
	console.log(err.message);
}