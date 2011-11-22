import os
import subprocess

srcdir = "."
blddir = "build"

def set_options(opt):
  opt.tool_options("compiler_cxx")

# You can use node-waf buildMinifier to compile minify.c,
# but note that it's done automatically by node-waf configure
def buildMinifier(ctxt):
  print "\n**** Compiling minify.c"
  cmd= "gcc "+ os.getcwd() + "/deps/minifier/src/minify.c -o "+ os.getcwd() + "/deps/minifier/bin/minify"
  print cmd
  subprocess.check_call(["sh", "-c", cmd])
  print ""

def cmd(filename,varName):
  minifier= os.getcwd() + "/deps/minifier/bin/minify"
  path= os.getcwd() + "/src/"
  str= "cat "+ path+ filename+ " | "+ minifier+ " "+ varName+ " > "+ path+ filename+ ".c"
  print str
  return str

# You can use node-waf minify to minify and C-ify the .js files,
# but note that it's done automatically by node-waf build/install
def minify(ctxt):
  print "\n**** Minifying .js source files"
  subprocess.check_call(["sh", "-c", cmd("createPool.js","kCreatePool_js")])
  print ""
  subprocess.check_call(["sh", "-c", cmd("events.js","kEvents_js")])
  print ""
  subprocess.check_call(["sh", "-c", cmd("load.js","kLoad_js")])
  print ""
  subprocess.check_call(["sh", "-c", cmd("thread_nextTick.js","kThread_nextTick_js")])
  print ""

def configure(conf):
  print ""
  print '******** THREADS_A_GOGO ********'
  print '**** Executing the configuration\n\n'
  conf.check_tool("gcc")
  conf.check_tool("compiler_cxx")
  print '\n**** Checking for node_addon\n'
  conf.check_tool("node_addon")
  buildMinifier(0)

def build(bld):
  print '\n*** Building the project'
  minify(0)
  
  obj = bld.new_task_gen("cxx", "shlib", "node_addon")
  obj.cxxflags = ["-g", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE", "-Wall", "-O0", "-Wunused-macros"]
  obj.target = "threads_a_gogo"
  obj.source = "src/threads_a_gogo.cc"