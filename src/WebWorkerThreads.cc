//2011-11 Proyectos Equis Ka, s.l., jorge@jorgechamorro.com
//WebWorkerThreads.cc


#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>
#include <uv.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "nan.h"
#include "nan_isolate_data_accessor.h"

#if defined(__unix__) || defined(__POSIX__) || defined(__APPLE__) || defined(_AIX)
#define WWT_PTHREAD 1
#include <pthread.h>
#include <unistd.h>
#ifndef uv_cond_t
#define uv_cond_signal(x) pthread_cond_signal(x)
#define uv_cond_init(x) pthread_cond_init(x, NULL)
#define uv_cond_wait(x,y) pthread_cond_wait(x, y)
typedef pthread_cond_t uv_cond_t;
#endif
#else
#define pthread_setcancelstate(x,y) NULL
#define pthread_setcanceltype(x,y) NULL
#endif


/*
static int debug_threads= 0;
static int debug_allocs= 0;
*/

#include "queues_a_gogo.cc"
#include "bson.cc"
#include "jslib.cc"

//using namespace node;
using namespace v8;

static Persistent<String> id_symbol;
static Persistent<ObjectTemplate> threadTemplate;
static bool useLocker;

static typeQueue* freeJobsQueue= NULL;
static typeQueue* freeThreadsQueue= NULL;

#define kThreadMagicCookie 0x99c0ffee
typedef struct {
  uv_async_t async_watcher; //MUST be the first one

  long int id;
  uv_thread_t thread;
  volatile int sigkill;

  typeQueue inQueue;  //Jobs to run
  typeQueue outQueue; //Jobs done

  volatile int IDLE;
  uv_cond_t IDLE_cv;
  uv_mutex_t IDLE_mutex;

  Isolate* isolate;
  Persistent<Context> context;
  Persistent<Object> JSObject;
  Persistent<Object> threadJSObject;
  Persistent<Object> dispatchEvents;

  unsigned long threadMagicCookie;
} typeThread;

enum jobTypes {
  kJobTypeEval,
  kJobTypeEvent,
  kJobTypeEventSerialized
};

typedef struct {
  int jobType;
  Persistent<Object> cb;
  union {
    struct {
      int length;
      String::Utf8Value* eventName;
      String::Utf8Value** argumentos;
    } typeEvent;
    struct {
      int length;
      String::Utf8Value* eventName;
      char* buffer;
      size_t bufferSize;
    } typeEventSerialized;
    struct {
      int error;
      int tiene_callBack;
      int useStringObject;
      String::Utf8Value* resultado;
      union {
        char* scriptText_CharPtr;
        String::Utf8Value* scriptText_StringObject;
      };
    } typeEval;
  };
} typeJob;

/*

cd deps/minifier/src
gcc minify.c -o minify
cat ../../../src/events.js | ./minify kEvents_js > ../../../src/kEvents_js
cat ../../../src/load.js | ./minify kLoad_js > ../../../src/kLoad_js
cat ../../../src/createPool.js | ./minify kCreatePool_js > ../../../src/kCreatePool_js
cat ../../../src/worker.js | ./minify kWorker_js > ../../../src/kWorker_js
cat ../../../src/thread_nextTick.js | ./minify kThread_nextTick_js > ../../../src/kThread_nextTick_js

*/

#include "events.js.c"
#include "load.js.c"
#include "createPool.js.c"
#include "worker.js.c"
#include "thread_nextTick.js.c"
//#include "JASON.js.c"

//node-waf configure uninstall distclean configure build install








static typeQueueItem* nuJobQueueItem (void) {
  typeQueueItem* qitem= queue_pull(freeJobsQueue);
  if (!qitem) {
    qitem= nuItem(kItemTypePointer, calloc(1, sizeof(typeJob)));
  }
  return qitem;
}






static typeThread* isAThread (Local<Object> receiver) {
  typeThread* thread;

  if (receiver->IsObject()) {
    if (receiver->InternalFieldCount() == 1) {
      thread= (typeThread*) NanGetInternalFieldPointer(receiver, 0);
      if (thread && (thread->threadMagicCookie == kThreadMagicCookie)) {
        return thread;
      }
    }
  }

  return NULL;
}






static void pushToInQueue (typeQueueItem* qitem, typeThread* thread) {
  uv_mutex_lock(&thread->IDLE_mutex);
  queue_push(qitem, &thread->inQueue);
  if (thread->IDLE) {
    uv_cond_signal(&thread->IDLE_cv);
  }
  uv_mutex_unlock(&thread->IDLE_mutex);
}






NAN_METHOD(Puts) {
  NanScope();
  int i= 0;
  while (i < args.Length()) {
    String::Utf8Value c_str(args[i]);
    fputs(*c_str, stdout);
    i++;
  }
  fflush(stdout);

  //fprintf(stdout, "*** Puts END\n");
  NanReturnUndefined();
}

NAN_METHOD(Print) {
	NanScope();
	int i= 0;
	while (i < args.Length()) {
		String::Utf8Value c_str(args[i]);
		fputs(*c_str, stdout);
		i++;
	}
	static char end = '\n';
	fputs(&end, stdout);
	fflush(stdout);
	
	//fprintf(stdout, "*** Puts END\n");
	NanReturnUndefined();
}




static void eventLoop (typeThread* thread);

// A background thread
#ifdef WWT_PTHREAD
static void* aThread (void* arg) {
#else
static void aThread (void* arg) {
#endif

  int dummy;
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &dummy);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &dummy);

  typeThread* thread= (typeThread*) arg;
  thread->isolate= Isolate::New();
  NanSetIsolateData(thread->isolate, thread);
  
  if (useLocker) {
	#if (NODE_MODULE_VERSION > 0x000B)
		v8::Locker myLocker(thread->isolate);
	#else
		v8::Locker myLocker(thread->isolate);
	#endif
    // I think it's not ok to create a isolate scope here,
    // because it will call Isolate::Exit automatically.
    //v8::Isolate::Scope isolate_scope(thread->isolate);
    eventLoop(thread);
  }
  else {
    //v8::Isolate::Scope isolate_scope(thread->isolate);
    eventLoop(thread);
  }
  //thread->isolate->Exit(); 
  thread->isolate->Dispose();
  
  // wake up callback
  if (!(thread->inQueue.length)) uv_async_send(&thread->async_watcher);
  
#ifdef WWT_PTHREAD
  return NULL;
#endif
}



//static Handle<Value> threadEmit (const Arguments &args);
//static Handle<Value> postMessage (const Arguments &args);
//static Handle<Value> postError (const Arguments &args);

NAN_METHOD(threadEmit);
NAN_METHOD(postMessage);
NAN_METHOD(postError);



static void eventLoop (typeThread* thread) {
  Isolate::Scope isolate_scope(thread->isolate);
  //thread->isolate->Enter();
  
  {
    NanScope();
    ExtensionConfiguration extensions(0, NULL);
	
	Local<FunctionTemplate> ftmpl = NanNew<FunctionTemplate>();
	Local<ObjectTemplate> otmpl = ftmpl->InstanceTemplate();
	Local<Context> ctx =  NanNew<Context>(&extensions, otmpl);

	//thread->context= Context::New();
	NanAssignPersistent(thread->context, ctx);
	ctx->Enter();


    Local<Object> global= NanNew(thread->context)->Global();

    Handle<Object> fs_obj = NanNew<Object>();
    JSObjFn(fs_obj, "readFileSync", readFileSync_);
    global->ForceSet(NanNew<String>("native_fs_"), fs_obj, attribute_ro_dd);

    Handle<Object> console_obj = NanNew<Object>();
    JSObjFn(console_obj, "log", console_log);
    JSObjFn(console_obj, "error", console_error);
    global->ForceSet(NanNew<String>("console"), console_obj, attribute_ro_dd);

    global->ForceSet(NanNew<String>("self"), global);
    global->ForceSet(NanNew<String>("global"), global);

    global->ForceSet(NanNew<String>("puts"), NanNew<FunctionTemplate>(Puts)->GetFunction());
    global->ForceSet(NanNew<String>("print"), NanNew<FunctionTemplate>(Print)->GetFunction());

    global->ForceSet(NanNew<String>("postMessage"), NanNew<FunctionTemplate>(postMessage)->GetFunction());
    global->ForceSet(NanNew<String>("__postError"), NanNew<FunctionTemplate>(postError)->GetFunction());

    Local<Object> threadObject= NanNew<Object>();
    global->ForceSet(NanNew<String>("thread"), threadObject);

    threadObject->Set(NanNew<String>("id"), NanNew<Number>(thread->id));
    threadObject->Set(NanNew<String>("emit"), NanNew<FunctionTemplate>(threadEmit)->GetFunction());
    Local<Object> dispatchEvents= Script::Compile(NanNew<String>(kEvents_js))->Run()->ToObject()->CallAsFunction(threadObject, 0, NULL)->ToObject();
    Local<Object> dispatchNextTicks= Script::Compile(NanNew<String>(kThread_nextTick_js))->Run()->ToObject();
    //Local<Array> _ntq= (v8::Array*) *threadObject->Get(NanNew<String>Symbol("_ntq"));
	//Local<Array> _ntq = Array::Cast(*threadObject->Get(NanNew<String>("_ntq")));
	Array* _ntq = Array::Cast(*threadObject->Get(NanNew<String>("_ntq")));

    Script::Compile(NanNew<String>(kLoad_js))->Run();

    double nextTickQueueLength= 0;
    long int ctr= 0;

    //SetFatalErrorHandler(FatalErrorCB);

    while (!thread->sigkill) {
      typeJob* job;
      typeQueueItem* qitem;

      {
        //NanScope();
        TryCatch onError;
        String::Utf8Value* str;
        Local<String> source;
        Local<Script> script;
        Local<Value> resultado;


        while ((qitem= queue_pull(&thread->inQueue))) {

          job= (typeJob*) qitem->asPtr;

          if ((++ctr) > 2e3) {
            ctr= 0;
            NanIdleNotification(1);

          }

          if (job->jobType == kJobTypeEval) {
            //Ejecutar un texto

            if (job->typeEval.useStringObject) {
              str= job->typeEval.scriptText_StringObject;
              source= NanNew<String>(**str, (*str).length());
              delete str;
            }
            else {
              source= NanNew<String>(job->typeEval.scriptText_CharPtr);
              free(job->typeEval.scriptText_CharPtr);
            }

            script= NanCompileScript(source);

            if (!onError.HasCaught()) resultado= NanRunScript(script);

            if (job->typeEval.tiene_callBack) {
              job->typeEval.error= onError.HasCaught() ? 1 : 0;
              job->typeEval.resultado= new String::Utf8Value(job->typeEval.error ? onError.Exception() : resultado);
              queue_push(qitem, &thread->outQueue);
              // wake up callback
              if (!(thread->inQueue.length)) uv_async_send(&thread->async_watcher);
            }
            else {
              queue_push(qitem, freeJobsQueue);
            }

            if (onError.HasCaught()) onError.Reset();
          }
          else if (job->jobType == kJobTypeEvent) {
            //Emitir evento.

            Local<Value> args[2];
            str= job->typeEvent.eventName;
            args[0]= NanNew<String>(**str, (*str).length());
            delete str;

            Local<Array> array= NanNew<Array>(job->typeEvent.length);
            args[1]= array;

            int i= 0;
            while (i < job->typeEvent.length) {
              str= job->typeEvent.argumentos[i];
              array->Set(i, NanNew<String>(**str, (*str).length()));
              delete str;
              i++;
            }

            free(job->typeEvent.argumentos);
            queue_push(qitem, freeJobsQueue);
            dispatchEvents->CallAsFunction(global, 2, args);
          }
          else if (job->jobType == kJobTypeEventSerialized) {
            Local<Value> args[2];
            str= job->typeEventSerialized.eventName;
            args[0]= NanNew<String>(**str, (*str).length());
            delete str;

      int len = job->typeEventSerialized.length;
      Local<Array> array= NanNew<Array>(len);
      args[1]= array;

        {
          BSON *bson = new BSON();
          char* data = job->typeEventSerialized.buffer;
          size_t size = job->typeEventSerialized.bufferSize;
          BSONDeserializer deserializer(bson, data, size);
          Local<Object> result = deserializer.DeserializeDocument(true)->ToObject();
          int i = 0; do { array->Set(i, result->Get(i)); } while (++i < len);
          free(data);
        }

            queue_push(qitem, freeJobsQueue);
            dispatchEvents->CallAsFunction(global, 2, args);
          }
        }

        if (_ntq->Length()) {

          if ((++ctr) > 2e3) {
            ctr= 0;
            NanIdleNotification(1);
          }

          resultado= dispatchNextTicks->CallAsFunction(global, 0, NULL);
          if (onError.HasCaught()) {
            nextTickQueueLength= 1;
            onError.Reset();
          }
          else {
            nextTickQueueLength= resultado->NumberValue();
          }
        }
      }

      if (nextTickQueueLength || thread->inQueue.length) continue;
      if (thread->sigkill) break;

      uv_mutex_lock(&thread->IDLE_mutex);
      if (!thread->inQueue.length) {
        thread->IDLE= 1;
        uv_cond_wait(&thread->IDLE_cv, &thread->IDLE_mutex);
        thread->IDLE= 0;
      }
      uv_mutex_unlock(&thread->IDLE_mutex);
    }
	
  }

  NanDisposePersistent(thread->context);
}






static void destroyaThread (typeThread* thread) {
  NanScope();

  thread->sigkill= 0;
  //TODO: hay que vaciar las colas y destruir los trabajos antes de ponerlas a NULL
  thread->inQueue.first= thread->inQueue.last= NULL;
  thread->outQueue.first= thread->outQueue.last= NULL;
  NanSetInternalFieldPointer(NanNew(thread->JSObject), 0, NULL);
  NanDisposePersistent(thread->JSObject);

  uv_unref((uv_handle_t*)&thread->async_watcher);

  if (freeThreadsQueue) {
    queue_push(nuItem(kItemTypePointer, thread), freeThreadsQueue);
  }
  else {
    free(thread);
  }
}






// C callback that runs in the main nodejs thread. This is the one responsible for
// calling the thread's JS callback.
static void Callback (uv_async_t *watcher, int revents) {
  typeThread* thread= (typeThread*) watcher;

  if (thread->sigkill) {
    destroyaThread(thread);
    return;
  }

  NanScope();
  typeJob* job;
  Local<Value> argv[2];
  Local<Value> null = NanNull();
  typeQueueItem* qitem;
  String::Utf8Value* str;

  TryCatch onError;
  while ((qitem= queue_pull(&thread->outQueue))) {
    job= (typeJob*) qitem->asPtr;

    if (job->jobType == kJobTypeEval) {

      if (job->typeEval.tiene_callBack) {
        str= job->typeEval.resultado;

        if (job->typeEval.error) {
          argv[0]= Exception::Error(NanNew<String>(**str, (*str).length()));
          argv[1]= null;
        } else {
          argv[0]= null;
          argv[1]= NanNew<String>(**str, (*str).length());
        }
        NanNew(job->cb)->CallAsFunction(NanNew(thread->JSObject), 2, argv);
        NanDisposePersistent(job->cb);
        job->typeEval.tiene_callBack= 0;

        delete str;
        job->typeEval.resultado= NULL;
      }

      queue_push(qitem, freeJobsQueue);

      if (onError.HasCaught()) {
        if (thread->outQueue.first) {
          uv_async_send(&thread->async_watcher); // wake up callback again
        }
        node::FatalException(onError);
        return;
      }
    }
    else if (job->jobType == kJobTypeEvent) {

      //fprintf(stdout, "*** Callback\n");

      Local<Value> args[2];

      str= job->typeEvent.eventName;
      args[0]= NanNew<String>(**str, (*str).length());
      delete str;

      Local<Array> array= NanNew<Array>(job->typeEvent.length);
      args[1]= array;

      int i= 0;
      while (i < job->typeEvent.length) {
        str= job->typeEvent.argumentos[i];
        array->Set(i, NanNew<String>(**str, (*str).length()));
        delete str;
        i++;
      }

      free(job->typeEvent.argumentos);
      queue_push(qitem, freeJobsQueue);
      NanNew(thread->dispatchEvents)->CallAsFunction(NanNew(thread->JSObject), 2, args);
    }
    else if (job->jobType == kJobTypeEventSerialized) {
      Local<Value> args[2];

      str= job->typeEventSerialized.eventName;
      args[0]= NanNew<String>(**str, (*str).length());
      delete str;

      int len = job->typeEventSerialized.length;
      Local<Array> array= NanNew<Array>(len);
      args[1]= array;

        {
          BSON *bson = new BSON();
          char* data = job->typeEventSerialized.buffer;
          size_t size = job->typeEventSerialized.bufferSize;
          BSONDeserializer deserializer(bson, data, size);
          Local<Object> result = deserializer.DeserializeDocument(true)->ToObject();
          int i = 0; do { array->Set(i, result->Get(i)); } while (++i < len);
          free(data);
        }

      queue_push(qitem, freeJobsQueue);
      NanNew(thread->dispatchEvents)->CallAsFunction(NanNew(thread->JSObject), 2, args);
    }
  }
}






// unconditionally destroys a thread by brute force.
NAN_METHOD(Destroy) {
  NanScope();
  //TODO: Hay que comprobar que this en un objeto y que tiene hiddenRefTotypeThread_symbol y que no es nil
  //TODO: Aquí habría que usar static void TerminateExecution(int thread_id);
  //TODO: static void v8::V8::TerminateExecution  ( Isolate *   isolate= NULL   )   [static]

  typeThread* thread= isAThread(args.This());
  if (!thread) {
    return NanThrowTypeError("thread.destroy(): the receiver must be a thread object");
  }

  if (!thread->sigkill) {
    //pthread_cancel(thread->thread);
    thread->sigkill= 1;
    uv_mutex_lock(&thread->IDLE_mutex);
    if (thread->IDLE) {
      uv_cond_signal(&thread->IDLE_cv);
    }
    uv_mutex_unlock(&thread->IDLE_mutex);
  }

  NanReturnUndefined();
}






// Eval: Pushes a job into the thread's ->inQueue.
NAN_METHOD(Eval){
  NanScope();

  if (!args.Length()) {
    return NanThrowTypeError("thread.eval(program [,callback]): missing arguments");
  }

  typeThread* thread= isAThread(args.This());
  if (!thread) {
    return NanThrowTypeError("thread.eval(): the receiver must be a thread object");
  }

  typeQueueItem* qitem= nuJobQueueItem();
  typeJob* job= (typeJob*) qitem->asPtr;

  job->typeEval.tiene_callBack= ((args.Length() > 1) && (args[1]->IsFunction()));
  if (job->typeEval.tiene_callBack) {
    Local<Object> local_cb = args[1]->ToObject();
	NanAssignPersistent(job->cb, local_cb);
  }
  job->typeEval.scriptText_StringObject= new String::Utf8Value(args[0]);
  job->typeEval.useStringObject= 1;
  job->jobType= kJobTypeEval;

  pushToInQueue(qitem, thread);
  NanReturnValue(args.This());
}





static char* readFile (Handle<String> path) {
  v8::String::Utf8Value c_str(path);
  FILE* fp= fopen(*c_str, "rb");
  if (!fp) {
    fprintf(stderr, "Error opening the file %s\n", *c_str);
    //@bruno: Shouldn't we throw, here ?
    return NULL;
  }
  fseek(fp, 0, SEEK_END);
  size_t len= ftell(fp);
  rewind(fp); //fseek(fp, 0, SEEK_SET);
  char *buf= (char*)malloc((len+1) * sizeof(char)); // +1 to get null terminated string
  if (fread(buf, sizeof(char), len, fp) < len) {
    fprintf(stderr, "Error reading the file %s\n", *c_str);
    return NULL;
  }
  buf[len] = 0;
  fclose(fp);
  /*
  printf("SOURCE:\n%s\n", buf);
  fflush(stdout);
  */
  return buf;
}






// Load: Loads from file and passes to Eval
NAN_METHOD(Load) {
  NanScope();

  if (!args.Length()) {
    return NanThrowTypeError("thread.load(filename [,callback]): missing arguments");
  }

  typeThread* thread= isAThread(args.This());
  if (!thread) {
    return NanThrowTypeError("thread.load(): the receiver must be a thread object");
  }

  char* source= readFile(args[0]->ToString());  //@Bruno: here we don't know if the file was not found or if it was an empty file
  if (!source) NanReturnValue(args.This()); //@Bruno: even if source is empty, we should call the callback ?

  typeQueueItem* qitem= nuJobQueueItem();
  typeJob* job= (typeJob*) qitem->asPtr;

  job->typeEval.tiene_callBack= ((args.Length() > 1) && (args[1]->IsFunction()));
  if (job->typeEval.tiene_callBack) {
    Local<Object> local_cb = args[1]->ToObject();
	NanAssignPersistent(job->cb, local_cb);
  }
  job->typeEval.scriptText_CharPtr= source;
  job->typeEval.useStringObject= 0;
  job->jobType= kJobTypeEval;

  pushToInQueue(qitem, thread);

  NanReturnValue(args.This());
}






NAN_METHOD(processEmit) {
  NanScope();

  if (!args.Length()) NanReturnValue(args.This());

  typeThread* thread= isAThread(args.This());
  if (!thread) {
    NanThrowTypeError("thread.emit(): the receiver must be a thread object");
  }

  typeQueueItem* qitem= nuJobQueueItem();
  typeJob* job= (typeJob*) qitem->asPtr;

  job->jobType= kJobTypeEvent;
  job->typeEvent.length= args.Length()- 1;
  job->typeEvent.eventName= new String::Utf8Value(args[0]);
  job->typeEvent.argumentos= (v8::String::Utf8Value**) malloc(job->typeEvent.length* sizeof(void*));

  int i= 1;
  do {
    job->typeEvent.argumentos[i-1]= new String::Utf8Value(args[i]);
  } while (++i <= job->typeEvent.length);

  pushToInQueue(qitem, thread);

  NanReturnValue(args.This());
}

NAN_METHOD(processEmitSerialized) {
  NanScope();

  int len = args.Length();

  if (!len) NanReturnValue(args.This());

  typeThread* thread= isAThread(args.This());
  if (!thread) {
    return NanThrowTypeError("thread.emit(): the receiver must be a thread object");
  }

  typeQueueItem* qitem= nuJobQueueItem();
  typeJob* job= (typeJob*) qitem->asPtr;

  job->jobType= kJobTypeEventSerialized;
  job->typeEventSerialized.length= len-1;
  job->typeEventSerialized.eventName= new String::Utf8Value(args[0]);
  Local<Array> array= NanNew<Array>(len-1);
  int i = 1; do { array->Set(i-1, args[i]); } while (++i < len);

    {
      char* buffer;
      BSON *bson = new BSON();
      size_t object_size;
      Local<Object> object = bson->GetSerializeObject(array);
      BSONSerializer<CountStream> counter(bson, false, false);
      counter.SerializeDocument(object);
      object_size = counter.GetSerializeSize();
      buffer = (char *)malloc(object_size);
      BSONSerializer<DataStream> data(bson, false, false, buffer);
      data.SerializeDocument(object);
      job->typeEventSerialized.buffer= buffer;
      job->typeEventSerialized.bufferSize= object_size;
    }

  pushToInQueue(qitem, thread);

  NanReturnValue(args.This());
}

#define POST_EVENT(eventname) { \
  NanScope(); \
  int len = args.Length(); \
 \
  if (!len) NanReturnValue(args.This()); \
 \
  typeThread* thread= (typeThread*) NanGetIsolateData(Isolate::GetCurrent()); \
 \
  typeQueueItem* qitem= nuJobQueueItem(); \
  typeJob* job= (typeJob*) qitem->asPtr; \
 \
  job->jobType= kJobTypeEventSerialized; \
  job->typeEventSerialized.eventName= new String::Utf8Value(NanNew<String>(eventname)); \
  job->typeEventSerialized.length= len; \
 \
  Local<Array> array= NanNew<Array>(len); \
  int i = 0; do { array->Set(i, args[i]); } while (++i < len); \
 \
    { \
      char* buffer; \
      BSON *bson = new BSON(); \
      size_t object_size; \
      Local<Object> object = bson->GetSerializeObject(array); \
      BSONSerializer<CountStream> counter(bson, false, false); \
      counter.SerializeDocument(object); \
      object_size = counter.GetSerializeSize(); \
      buffer = (char *)malloc(object_size); \
      BSONSerializer<DataStream> data(bson, false, false, buffer); \
      data.SerializeDocument(object); \
      job->typeEventSerialized.buffer= buffer; \
      job->typeEventSerialized.bufferSize= object_size; \
    } \
 \
  queue_push(qitem, &thread->outQueue); \
  if (!(thread->inQueue.length)) uv_async_send(&thread->async_watcher); \
 \
  NanReturnValue(args.This()); \
}

NAN_METHOD(postMessage) {
  POST_EVENT("message");
}

NAN_METHOD(postError) {
  POST_EVENT("error");
}

NAN_METHOD(threadEmit) {
  NanScope();

  if (!args.Length()) NanReturnValue(args.This());

  int i;
  typeThread* thread= (typeThread*) NanGetIsolateData(Isolate::GetCurrent());

  typeQueueItem* qitem= nuJobQueueItem();
  typeJob* job= (typeJob*) qitem->asPtr;

  job->jobType= kJobTypeEvent;
  job->typeEvent.length= args.Length()- 1;
  job->typeEvent.eventName= new String::Utf8Value(args[0]);
  job->typeEvent.argumentos= (v8::String::Utf8Value**) malloc(job->typeEvent.length* sizeof(void*));

  i= 1;
  do {
    job->typeEvent.argumentos[i-1]= new String::Utf8Value(args[i]);
  } while (++i <= job->typeEvent.length);

  queue_push(qitem, &thread->outQueue);
  if (!(thread->inQueue.length)) uv_async_send(&thread->async_watcher); // wake up callback

  NanReturnValue(args.This());
}








// Creates and launches a new isolate in a new background thread.
NAN_METHOD(Create) {
    NanScope();

    typeThread* thread;
    typeQueueItem* qitem= NULL;
    qitem= queue_pull(freeThreadsQueue);
    if (qitem) {
      thread= (typeThread*) qitem->asPtr;
      destroyItem(qitem);
    }
    else {
      thread= (typeThread*) calloc(1, sizeof(typeThread));
      thread->threadMagicCookie= kThreadMagicCookie;
    }

    static long int threadsCtr= 0;
    thread->id= threadsCtr++;

    Local<Object> local_JSObject = NanNew(threadTemplate)->NewInstance();
    local_JSObject->Set(NanNew(id_symbol), NanNew<Integer>((int32_t)thread->id));
	NanSetInternalFieldPointer(local_JSObject, 0, thread);
	NanAssignPersistent(thread->JSObject, local_JSObject);
	
    Local<Value> dispatchEvents= Script::Compile(NanNew<String>(kEvents_js))->Run()->ToObject()->CallAsFunction(local_JSObject, 0, NULL);
	Local<Object> local_dispatchEvents = dispatchEvents->ToObject();
	NanAssignPersistent(thread->dispatchEvents, local_dispatchEvents);

    uv_async_init(uv_default_loop(), &thread->async_watcher, reinterpret_cast<uv_async_cb>(Callback));
    uv_ref((uv_handle_t*)&thread->async_watcher);

    uv_cond_init(&thread->IDLE_cv);
    uv_mutex_init(&thread->IDLE_mutex);
    uv_mutex_init(&thread->inQueue.queueLock);
    uv_mutex_init(&thread->outQueue.queueLock);

#ifdef WWT_PTHREAD
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    int err= pthread_create(&thread->thread, &attr, aThread, thread);
    pthread_attr_destroy(&attr);
#else
    int err= uv_thread_create(&thread->thread, aThread, thread);
#endif
    if (err) {
      //Ha habido un error no se ha arrancado esta thread
      destroyaThread(thread);
      return NanThrowTypeError("create(): error in pthread_create()");
    }

    NanAdjustExternalMemory(sizeof(typeThread));
    NanReturnValue(NanNew(thread->JSObject));
}


#if NODE_MODULE_VERSION >= 0x000E
void Init (Handle<Object> target, Handle<Value> module, void *) {
#elif NODE_MODULE_VERSION >= 0x000B
void Init (Handle<Object> target, Handle<Value> module) {
#else
void Init (Handle<Object> target) {
#endif

  initQueues();
  freeThreadsQueue= nuQueue(-3);
  freeJobsQueue= nuQueue(-4);

  NanScope();

  useLocker= v8::Locker::IsActive();

  target->Set(NanNew<String>("create"), NanNew<FunctionTemplate>(Create)->GetFunction());
  target->Set(NanNew<String>("createPool"), Script::Compile(NanNew<String>(kCreatePool_js))->Run()->ToObject());
  target->Set(NanNew<String>("Worker"), Script::Compile(NanNew<String>(kWorker_js))->Run()->ToObject()->CallAsFunction(target, 0, NULL)->ToObject());
  
  Local<String> local_id_symbol = NanNew<String>("id");

  Local<ObjectTemplate> local_threadTemplate = ObjectTemplate::New();
  local_threadTemplate->SetInternalFieldCount(1);
  local_threadTemplate->Set(local_id_symbol, NanNew<Integer>(0));
  NanAssignPersistent(id_symbol, local_id_symbol);
  local_threadTemplate->Set(NanNew<String>("eval"), NanNew<FunctionTemplate>(Eval));
  local_threadTemplate->Set(NanNew<String>("load"), NanNew<FunctionTemplate>(Load));
  local_threadTemplate->Set(NanNew<String>("emit"), NanNew<FunctionTemplate>(processEmit));
  local_threadTemplate->Set(NanNew<String>("emitSerialized"), NanNew<FunctionTemplate>(processEmitSerialized));
  local_threadTemplate->Set(NanNew<String>("destroy"), NanNew<FunctionTemplate>(Destroy));
  NanAssignPersistent(threadTemplate, local_threadTemplate);
}




NODE_MODULE(WebWorkerThreads, Init)

/*
gcc -E -I /Users/jorge/JAVASCRIPT/binarios/include/node -o /o.c /Users/jorge/JAVASCRIPT/threads_a_gogo/src/threads_a_gogo.cc && mate /o.c
*/
