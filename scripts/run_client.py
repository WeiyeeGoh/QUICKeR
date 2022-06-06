import subprocess
import sys
import os
import time
import datetime

def run_local_task(cmd):
  p = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                        shell=True, preexec_fn=os.setsid)
  out, err = p.communicate()
  print("Output")

def run_experiment(logname):
  print(logname)

  test_name = sys.argv[1]

  cmd = '../build/src/main/' + test_name + ' arguments.txt > ' + logname
  print(cmd)
  run_local_task(cmd)
    
if len(sys.argv) < 3:
  print("Format: <test_name> <logname>")

run_experiment(sys.argv[2])
