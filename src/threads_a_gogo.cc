//2011-11 Proyectos Equis Ka, s.l., jorge@jorgechamorro.com
//threads_a_gogo.cc

#include <v8.h>
#include <node.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string>

/*
static int debug_threads= 0;
static int debug_allocs= 0;
*/

#include "queues_a_gogo.cc"

//using namespace node;
using namespace v8;

static Persistent<String> id_symbol;
static Persistent<ObjectTemplate> threadTemplate;
static bool useLocker;

static typeQueue* freeJobsQueue= NULL;
static typeQueue* freeThreadsQueue= NULL;

#define kThreadMagicCookie 0x99c0ffee
typedef struct {
  ev_async async_watcher; //MUST be the first one
  
  long int id;
  pthread_t thread;
  volatile int sigkill;
  
  typeQueue inQueue;  //Jobs to run
  typeQueue outQueue; //Jobs done
  
  volatile int IDLE;
  pthread_cond_t IDLE_cv;
  pthread_mutex_t IDLE_mutex;
  
  Isolate* isolate;
  Persistent<Context> context;
  Persistent<Object> parentJSObject;
  Persistent<Object> parentDispatchEvents;
  
  unsigned long threadMagicCookie;
} typeThread;

typedef struct Weak {
  typeThread* thread;
  int isWeak;
  struct Weak* next;
} typeWeak;

typedef struct SharedStr {
  char* data;
  ssize_t len;
  typeWeak* weak;
} typeSharedStr;

enum jobTypes {
  kJobTypeEval,
  kJobTypeEvent
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
cat ../../../src/thread_nextTick.js | ./minify kThread_nextTick_js > ../../../src/kThread_nextTick_js

*/

#include "events.js.c"
//#include "load.js.c"
#include "createPool.js.c"
#include "thread_nextTick.js.c"
//#include "JASON.js.c"

//node-waf configure uninstall distclean configure build install


static typeQueueItem* nuJobQueueItem (void);
static typeThread* isAThread (Handle<Object> receiver);
static void pushToInQueue (typeQueueItem* qitem, typeThread* thread);
static Handle<Value> Puts (const Arguments &args);
static void* aThread (void* arg);
static void eventLoop (typeThread* thread);
static void destroyaThread (typeThread* thread);
static void Callback (EV_P_ ev_async *watcher, int revents);
static Handle<Value> Destroy (const Arguments &args);
static Handle<Value> Eval (const Arguments &args);
static char* readFile (Handle<String> path);
static Handle<Value> Load (const Arguments &args);
static Handle<Value> processEmit (const Arguments &args);
static Handle<Value> threadEmit (const Arguments &args);
static Handle<Value> Create (const Arguments &args);
void Init (Handle<Object> target);






static typeQueueItem* nuJobQueueItem (void) {
  typeQueueItem* qitem= queue_pull(freeJobsQueue);
  if (!qitem) {
    qitem= nuItem(kItemTypePointer, calloc(1, sizeof(typeJob)));
  }
  return qitem;
}






static typeThread* isAThread (Handle<Object> receiver) {
  typeThread* thread;
  
  if (receiver->IsObject()) {
    if (receiver->InternalFieldCount() == 1) {
      thread= (typeThread*) receiver->GetPointerFromInternalField(0);
      if (thread && (thread->threadMagicCookie == kThreadMagicCookie)) {
        return thread;
      }
    }
  }
  
  return NULL;
}






static void pushToInQueue (typeQueueItem* qitem, typeThread* thread) {
  pthread_mutex_lock(&thread->IDLE_mutex);
  queue_push(qitem, &thread->inQueue);
  if (thread->IDLE) {
    pthread_cond_signal(&thread->IDLE_cv);
  }
  pthread_mutex_unlock(&thread->IDLE_mutex);
}






static Handle<Value> Puts (const Arguments &args) {
  //fprintf(stdout, "*** Puts BEGIN\n");
  
  HandleScope scope;
  int i= 0;
  while (i < args.Length()) {
    String::Utf8Value c_str(args[i]);
    fputs(*c_str, stdout);
    i++;
  }
  fflush(stdout);

  //fprintf(stdout, "*** Puts END\n");
  return Undefined();
}






// A background thread
static void* aThread (void* arg) {
  
  int dummy;
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &dummy);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &dummy);
  
  typeThread* thread= (typeThread*) arg;
  
  V8::SetFlagsFromString("--expose_gc", 11);
  thread->isolate= Isolate::New();
  thread->isolate->SetData(thread);
  
  if (useLocker) {
    //printf("**** USING LOCKER: YES\n");
    v8::Locker myLocker(thread->isolate);
    //v8::Isolate::Scope isolate_scope(thread->isolate);
    eventLoop(thread);
  }
  else {
    //printf("**** USING LOCKER: NO\n");
    //v8::Isolate::Scope isolate_scope(thread->isolate);
    eventLoop(thread);
  }
  thread->isolate->Exit();
  thread->isolate->Dispose();
  
  // wake up callback
  if (!ev_async_pending(&thread->async_watcher)) ev_async_send(EV_DEFAULT_UC_ &thread->async_watcher);
  
  return NULL;
}






static void eventLoop (typeThread* thread) {
  thread->isolate->Enter();
  thread->context= Context::New();
  thread->context->Enter();
  
  {
    HandleScope scope1;
    
    Local<Object> global= thread->context->Global();
    global->Set(String::NewSymbol("puts"), FunctionTemplate::New(Puts)->GetFunction());
    Local<Object> childJSObject= Object::New();
    global->Set(String::NewSymbol("thread"), childJSObject);
    
    childJSObject->Set(String::NewSymbol("id"), Number::New(thread->id));
    childJSObject->Set(String::NewSymbol("emit"), FunctionTemplate::New(threadEmit)->GetFunction());
    Local<Object> childDispatchEvents= Script::Compile(String::New(kEvents_js))->Run()->ToObject()->CallAsFunction(childJSObject, 0, NULL)->ToObject();
    Local<Object> dispatchNextTicks= Script::Compile(String::New(kThread_nextTick_js))->Run()->ToObject();
    Local<Array> _ntq= (v8::Array*) *childJSObject->Get(String::NewSymbol("_ntq"));
    
    double nextTickQueueLength= 0;
    long int ctr= 0;
    
    //SetFatalErrorHandler(FatalErrorCB);
    
    while (!thread->sigkill) {
      typeJob* job;
      typeQueueItem* qitem;
      
      {
        HandleScope scope2;
        TryCatch onError;
        String::Utf8Value* str;
        Local<String> source;
        Local<Script> script;
        Local<Value> resultado;
        
        
        while ((qitem= queue_pull(&thread->inQueue))) {
          
          job= (typeJob*) qitem->asPtr;
          
          if ((++ctr) > 2e3) {
            ctr= 0;
            V8::IdleNotification();
          }
          
          if (job->jobType == kJobTypeEval) {
            //Ejecutar un texto
            
            if (job->typeEval.useStringObject) {
              str= job->typeEval.scriptText_StringObject;
              source= String::New(**str, (*str).length());
              delete str;
            }
            else {
              source= String::New(job->typeEval.scriptText_CharPtr);
              free(job->typeEval.scriptText_CharPtr);
            }
            
            script= Script::New(source);
            
            if (!onError.HasCaught()) resultado= script->Run();

            if (job->typeEval.tiene_callBack) {
              job->typeEval.error= onError.HasCaught() ? 1 : 0;
              job->typeEval.resultado= new String::Utf8Value(job->typeEval.error ? onError.Exception() : resultado);
              queue_push(qitem, &thread->outQueue);
              // wake up callback
              if (!ev_async_pending(&thread->async_watcher)) ev_async_send(EV_DEFAULT_UC_ &thread->async_watcher);
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
            args[0]= String::New(**str, (*str).length());
            delete str;
            
            Local<Array> array= Array::New(job->typeEvent.length);
            args[1]= array;
            
            int i= 0;
            while (i < job->typeEvent.length) {
              str= job->typeEvent.argumentos[i];
              array->Set(i, String::New(**str, (*str).length()));
              delete str;
              i++;
            }
            
            free(job->typeEvent.argumentos);
            queue_push(qitem, freeJobsQueue);
            childDispatchEvents->CallAsFunction(global, 2, args);
          }
        }
        
        if (_ntq->Length()) {
          
          if ((++ctr) > 2e3) {
            ctr= 0;
            V8::IdleNotification();
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
      
      pthread_mutex_lock(&thread->IDLE_mutex);
      if (!thread->inQueue.length) {
        thread->IDLE= 1;
        pthread_cond_wait(&thread->IDLE_cv, &thread->IDLE_mutex);
        thread->IDLE= 0;
      }
      pthread_mutex_unlock(&thread->IDLE_mutex);
    }
  }
  
  thread->context.Dispose();
}






static void destroyaThread (typeThread* thread) {
  
  thread->sigkill= 0;
  //TODO: hay que vaciar las colas y destruir los trabajos antes de resetearlas
  resetQueue(&thread->inQueue);
  resetQueue(&thread->outQueue);
  thread->parentJSObject->SetPointerInInternalField(0, NULL);
  thread->parentJSObject.Dispose();
  
  ev_async_stop(EV_DEFAULT_UC_ &thread->async_watcher);
  ev_unref(EV_DEFAULT_UC);
  
  if (freeThreadsQueue) {
    queue_push(nuItem(kItemTypePointer, thread), freeThreadsQueue);
  }
  else {
    free(thread);
  }
}






// C callback that runs in the main nodejs thread. This is the one responsible for
// calling the thread's JS callback.
static void Callback (EV_P_ ev_async *watcher, int revents) {
  typeThread* thread= (typeThread*) watcher;
  
  if (thread->sigkill) {
    destroyaThread(thread);
    return;
  }
  
  HandleScope scope;
  typeJob* job;
  Local<Value> argv[2];
  Local<Value> null= Local<Value>::New(Null());
  typeQueueItem* qitem;
  String::Utf8Value* str;
  
  TryCatch onError;
  while ((qitem= queue_pull(&thread->outQueue))) {
    job= (typeJob*) qitem->asPtr;

    if (job->jobType == kJobTypeEval) {

      if (job->typeEval.tiene_callBack) {
        str= job->typeEval.resultado;

        if (job->typeEval.error) {
          argv[0]= Exception::Error(String::New(**str, (*str).length()));
          argv[1]= null;
        } else {
          argv[0]= null;
          argv[1]= String::New(**str, (*str).length());
        }
        job->cb->CallAsFunction(thread->parentJSObject, 2, argv);
        job->cb.Dispose();
        job->typeEval.tiene_callBack= 0;

        delete str;
        job->typeEval.resultado= NULL;
      }

      queue_push(qitem, freeJobsQueue);
      
      if (onError.HasCaught()) {
        if (thread->outQueue.first) {
          ev_async_send(EV_DEFAULT_UC_ &thread->async_watcher); // wake up callback again
        }
        node::FatalException(onError);
        return;
      }
    }
    else if (job->jobType == kJobTypeEvent) {
      
      //fprintf(stdout, "*** Callback\n");
      
      Local<Value> args[2];
      
      str= job->typeEvent.eventName;
      args[0]= String::New(**str, (*str).length());
      delete str;
      
      Local<Array> array= Array::New(job->typeEvent.length);
      args[1]= array;
      
      int i= 0;
      while (i < job->typeEvent.length) {
        str= job->typeEvent.argumentos[i];
        array->Set(i, String::New(**str, (*str).length()));
        delete str;
        i++;
      }
      
      free(job->typeEvent.argumentos);
      queue_push(qitem, freeJobsQueue);
      thread->parentDispatchEvents->CallAsFunction(thread->parentJSObject, 2, args);
    }
  }
}






// unconditionally destroys a thread by brute force.
static Handle<Value> Destroy (const Arguments &args) {
  HandleScope scope;
  //TODO: Hay que comprobar que this en un objeto y que tiene hiddenRefTotypeThread_symbol y que no es nil
  //TODO: Aquí habría que usar static void TerminateExecution(int thread_id);
  //TODO: static void v8::V8::TerminateExecution  ( Isolate *   isolate= NULL   )   [static]
  
  typeThread* thread= isAThread(args.This());
  if (!thread) {
    return ThrowException(Exception::TypeError(String::New("thread.destroy(): the receiver must be a thread object")));
  }
  
  if (!thread->sigkill) {
    //pthread_cancel(thread->thread);
    thread->sigkill= 1;
    pthread_mutex_lock(&thread->IDLE_mutex);
    if (thread->IDLE) {
      pthread_cond_signal(&thread->IDLE_cv);
    }
    pthread_mutex_unlock(&thread->IDLE_mutex);
  }
  
  return Undefined();
}






// Eval: Pushes a job into the thread's ->inQueue.
static Handle<Value> Eval (const Arguments &args) {
  HandleScope scope;
  
  if (!args.Length()) {
    return ThrowException(Exception::TypeError(String::New("thread.eval(program [,callback]): missing arguments")));
  }
  
  typeThread* thread= isAThread(args.This());
  if (!thread) {
    return ThrowException(Exception::TypeError(String::New("thread.eval(): the receiver must be a thread object")));
  }

  typeQueueItem* qitem= nuJobQueueItem();
  typeJob* job= (typeJob*) qitem->asPtr;
  
  job->typeEval.tiene_callBack= ((args.Length() > 1) && (args[1]->IsFunction()));
  if (job->typeEval.tiene_callBack) {
    job->cb= Persistent<Object>::New(args[1]->ToObject());
  }
  job->typeEval.scriptText_StringObject= new String::Utf8Value(args[0]);
  job->typeEval.useStringObject= 1;
  job->jobType= kJobTypeEval;
  
  pushToInQueue(qitem, thread);
  return scope.Close(args.This());
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
  long len= ftell(fp);
  rewind(fp); //fseek(fp, 0, SEEK_SET);
  char *buf= (char*) calloc(len + 1, sizeof(char)); // +1 to get null terminated string
  fread(buf, len, 1, fp);
  fclose(fp);
  /*
  printf("SOURCE:\n%s\n", buf);
  fflush(stdout);
  */
  return buf;
}






// Load: Loads from file and passes to Eval
static Handle<Value> Load (const Arguments &args) {
  HandleScope scope;

  if (!args.Length()) {
    return ThrowException(Exception::TypeError(String::New("thread.load(filename [,callback]): missing arguments")));
  }

  typeThread* thread= isAThread(args.This());
  if (!thread) {
    return ThrowException(Exception::TypeError(String::New("thread.load(): the receiver must be a thread object")));
  }
  
  char* source= readFile(args[0]->ToString());  //@Bruno: here we don't know if the file was not found or if it was an empty file
  if (!source) return scope.Close(args.This()); //@Bruno: even if source is empty, we should call the callback ?

  typeQueueItem* qitem= nuJobQueueItem();
  typeJob* job= (typeJob*) qitem->asPtr;

  job->typeEval.tiene_callBack= ((args.Length() > 1) && (args[1]->IsFunction()));
  if (job->typeEval.tiene_callBack) {
    job->cb= Persistent<Object>::New(args[1]->ToObject());
  }
  job->typeEval.scriptText_CharPtr= source;
  job->typeEval.useStringObject= 0;
  job->jobType= kJobTypeEval;

  pushToInQueue(qitem, thread);

  return scope.Close(args.This());
}






static Handle<Value> processEmit (const Arguments &args) {
  HandleScope scope;
  
  //fprintf(stdout, "*** processEmit\n");
  
  if (!args.Length()) return scope.Close(args.This());
  
  typeThread* thread= isAThread(args.This());
  if (!thread) {
    return ThrowException(Exception::TypeError(String::New("thread.emit(): the receiver must be a thread object")));
  }
  
  typeQueueItem* qitem= nuJobQueueItem();
  typeJob* job= (typeJob*) qitem->asPtr;
  
  job->jobType= kJobTypeEvent;
  job->typeEvent.length= args.Length()- 1;
  
  
  /*
  int isExternal= String::Cast(*args[0])->IsExternal();
  printf("BEFORE: isExternal() -> %d\n", isExternal);
  String::ExternalStringResource* externalStrRsrc;
  if (!isExternal && String::Cast(*args[0])->CanMakeExternal()) {
    printf("eventName->MakeExternal(externalStrRsrc)\n");
    isExternal= String::Cast(*args[0])->MakeExternal(externalStrRsrc);
  }
  printf("AFTER: .isExternal() -> %d, .length() -> %d\n", isExternal, externalStrRsrc == NULL);
  */
  
  
  job->typeEvent.eventName= new String::Utf8Value(args[0]);
  job->typeEvent.argumentos= (v8::String::Utf8Value**) malloc(job->typeEvent.length* sizeof(void*));
  
  int i= 1;
  do {
    job->typeEvent.argumentos[i-1]= new String::Utf8Value(args[i]);
  } while (++i <= job->typeEvent.length);
  
  pushToInQueue(qitem, thread);
  
  return scope.Close(args.This());
}






static Handle<Value> threadEmit (const Arguments &args) {
  HandleScope scope;
  
  //fprintf(stdout, "*** threadEmit\n");
  
  if (!args.Length()) return scope.Close(args.This());
  
  int i;
  typeThread* thread= (typeThread*) Isolate::GetCurrent()->GetData();
  
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
  if (!ev_async_pending(&thread->async_watcher)) ev_async_send(EV_DEFAULT_UC_ &thread->async_watcher); // wake up callback
  
  //fprintf(stdout, "*** threadEmit END\n");
  
  return scope.Close(args.This());
}






// Creates and launches a new isolate in a new background thread.
static Handle<Value> Create (const Arguments &args) {
    HandleScope scope;
    
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
    
    thread->parentJSObject= Persistent<Object>::New(threadTemplate->NewInstance());
    thread->parentJSObject->Set(id_symbol, Integer::New(thread->id));
    thread->parentJSObject->SetPointerInInternalField(0, thread);
    Local<Value> parentDispatchEvents= Script::Compile(String::New(kEvents_js))->Run()->ToObject()->CallAsFunction(thread->parentJSObject, 0, NULL);
    thread->parentDispatchEvents= Persistent<Object>::New(parentDispatchEvents->ToObject());
    
    ev_async_init(&thread->async_watcher, Callback);
    ev_async_start(EV_DEFAULT_UC_ &thread->async_watcher);
    ev_ref(EV_DEFAULT_UC);
    
    pthread_cond_init(&thread->IDLE_cv, NULL);
    pthread_mutex_init(&thread->IDLE_mutex, NULL);
    pthread_mutex_init(&thread->inQueue.queueLock, NULL);
    pthread_mutex_init(&thread->outQueue.queueLock, NULL);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    int err= pthread_create(&thread->thread, &attr, aThread, thread);
    pthread_attr_destroy(&attr);
    if (err) {
      //Ha habido un error no se ha arrancado esta thread
      destroyaThread(thread);
      return ThrowException(Exception::TypeError(String::New("create(): error in pthread_create()")));
    }
    
    V8::AdjustAmountOfExternalAllocatedMemory(sizeof(typeThread));  //OJO V8 con V mayúscula.
    return scope.Close(thread->parentJSObject);
}





void Init (Handle<Object> target) {
  
  initQueues();
  freeThreadsQueue= nuQueue(-3);
  freeJobsQueue= nuQueue(-4);
  
  HandleScope scope;
  
  useLocker= v8::Locker::IsActive();
  
  target->Set(String::NewSymbol("create"), FunctionTemplate::New(Create)->GetFunction());
  target->Set(String::NewSymbol("createPool"), Script::Compile(String::New(kCreatePool_js))->Run()->ToObject());
  //target->Set(String::NewSymbol("JASON"), Script::Compile(String::New(kJASON_js))->Run()->ToObject());
  
  id_symbol= Persistent<String>::New(String::NewSymbol("id"));

  threadTemplate= Persistent<ObjectTemplate>::New(ObjectTemplate::New());
  threadTemplate->SetInternalFieldCount(1);
  threadTemplate->Set(id_symbol, Integer::New(0));
  threadTemplate->Set(String::NewSymbol("eval"), FunctionTemplate::New(Eval));
  threadTemplate->Set(String::NewSymbol("load"), FunctionTemplate::New(Load));
  threadTemplate->Set(String::NewSymbol("emit"), FunctionTemplate::New(processEmit));
  threadTemplate->Set(String::NewSymbol("destroy"), FunctionTemplate::New(Destroy));
  
}





NODE_MODULE(threads_a_gogo, Init)

/*
gcc -E -I /Users/jorge/JAVASCRIPT/binarios/include/node -o /o.c /Users/jorge/JAVASCRIPT/threads_a_gogo/src/threads_a_gogo.cc && mate /o.c
*/