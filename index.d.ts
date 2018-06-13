
export interface Event {
	data: any;
}

export type CallBack = (...args: any[]) => any;

export interface WorkerConstructor {
	new(jsfilepath: string): Worker;
	new(fun: (this: Worker) => void): Worker;
	new(): Worker;
}

export interface Worker {
	// Returns the underlying thread object; see the next section for details. Note that this attribute is implementation-specific, and not part of W3C Web Worker API.
	readonly thread: Thread;
	// worker.postMessage({ x: 1, y: 2 }) sends a data structure into the worker. The worker can receive it using the onmessage handler.
	postMessage(data: any): void;
	// worker.onmessage = function (event) { console.log(event.data) }; receives data from the worker's postMessage calls.
	onmessage(event: Event): void;
	// terminates the worker thread.
	terminate(): void;
	// worker.addEventListener('message', callback) is equivalent to setting worker.onmesssage = callback.
	addEventListener(type: string, cb: CallBack): void;
	// Currently unimplemented.
	dispatchEvent(type: string);
	// Currently unimplemented.
	removeEventListener(type: string);
}

export interface Thread {
	readonly id: number;
	// thread.load( absolutePath [, cb] ) reads the file at absolutePath and thread.eval(fileContents, cb).
	load(absolutePath: string, cb?: (this: Thread, err: any, data: any) => void): void;

	// thread.eval( program [, cb]) converts program.toString() and eval()s it in the thread's global context, and (if provided) returns the completion value to cb(err, completionValue).
	eval<T extends { toString(): string; }>(program: T, cb?: (this: Thread, err: any, data: any) => void): this;

	// thread.on( eventType, listener ) registers the listener listener(data) for any events of eventType that the thread thread may emit.
	on(eventType: string, listener: (data: any) => void): this;

	// thread.once(eventType, listener) is like thread.on(), but the listener will only be called once.
	once(eventType: string, listener: (data: any) => void): this;

	// thread.removeAllListeners([eventType]) deletes all listeners for all eventTypes.If eventType is provided, deletes all listeners only for the event type eventType.
	removeAllListeners(eventType?: string): this;

	// thread.emit(eventType, eventData[, eventData ... ]) emits an event of eventType with eventData inside the thread thread.All its arguments are.toString()ed.
	emit(eventType: string, ...eventDatas: any[]): this;

	// thread.destroy( /* no arguments */) destroys the thread.
	destroy(): this;
}

export interface ThreadPool {
	// threadPool.load( absolutePath [, cb] ) runs thread.load( absolutePath [, cb] ) in all the pool's threads.
	load(absolutePath: string, cb?: (this: Thread, err: any, data: any) => void): this;

	readonly any: {
		// threadPool.any.eval( program, cb ) is like thread.eval(), but in any of the pool's threads.
		eval(program: any, cb?: (this: Thread, err: any, data: any) => void): ThreadPool;

		// threadPool.any.emit( eventType, eventData [, eventData ... ] ) is like thread.emit(), but in any of the pool's threads.
		emit(eventType: string, ...eventData: any[]): ThreadPool;
	}

	readonly all: {
		// threadPool.all.eval( program, cb ) is like thread.eval(), but in all the pool's threads.
		eval(program: any, cb?: (this: Thread, err: any, data: any) => void): ThreadPool;

		// threadPool.all.emit( eventType, eventData [, eventData ... ] ) is like thread.emit(), but in all the pool's threads.
		emit(eventType: string, ...eventData: any[]): ThreadPool;
	}


	// threadPool.on( eventType, listener ) is like thread.on(), but in all of the pool's threads.
	on(eventType: string, listener: CallBack): this;

	// threadPool.totalThreads() returns the number of threads in this pool: as supplied in .createPool( number )
	totalThreads(): number;

	// threadPool.idleThreads() returns the number of threads in this pool that are currently idle (sleeping)
	idleThreads(): number;

	// threadPool.pendingJobs() returns the number of jobs pending.
	pendingJobs(): number;

	// threadPool.destroy( [ rudely ] ) waits until pendingJobs() is zero and then destroys the pool. If rudely is truthy, then it doesn't wait for pendingJobs === 0.
	destroy(rudely?: boolean): void;
}

export const Worker: WorkerConstructor;

export function createPool(numThreads: number): ThreadPool;

export function create(): Thread;
