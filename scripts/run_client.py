import subprocess
import sys
import os
import time
import datetime

def run_local_task(cmd):

  # with open("stdout.txt","wb") as out, open("stderr.txt","wb") as err:
  #   p = subprocess.Popen(cmd, stdout=out, stderr=err, shell=True)


  #f = open("stdout.txt", "w")
  p = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                        shell=True, preexec_fn=os.setsid)
  out, err = p.communicate()
  print(cmd)  

  # print("Output")
  # print(cmd.split(" "))
  # subprocess.run(cmd.split(" "))

def run_experiment(logname):
  print(logname)

  test_name = sys.argv[1]

  cmd = '../build/src/main/' + test_name + ' arguments.txt Output.txt >' + logname
  print(cmd)
  run_local_task(cmd)
    
if len(sys.argv) < 3:
  print("Format: <test_name> <logname>")

run_experiment(sys.argv[2])
