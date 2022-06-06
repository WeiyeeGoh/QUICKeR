import subprocess
import sys
import os
import time

def setup_local_task(cmd):
  p = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                        shell=True, preexec_fn=os.setsid)
  out, err = p.communicate()

  print(out)

def setup_experiment():
  cmd = "cd ..;rm -r build; mkdir build; cd build; cmake ..; make; cd .."

  print(cmd)
  setup_local_task(cmd)

def setup_hsms():
  print(sys.argv[1])
  #cmd = "sudo service cloudhsm-client stop; sudo /opt/cloudhsm/bin/configure -a " + sys.argv[1] + "; sudo service cloudhsm-client start"
  
  subprocess.run(["sudo", "service", "cloudhsm-client", "stop"])
  subprocess.run(["sudo", "/opt/cloudhsm/bin/configure", "-a", sys.argv[1]])
  subprocess.run(["sudo", "service", "cloudhsm-client", "start"])



if len(sys.argv) < 2:
  print("Need args for hsm ip-address")
  exit(0)

setup_experiment()
setup_hsms()