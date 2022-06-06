import subprocess
import sys
import os
import time
import datetime


client_machines = "routine_operations"
update_machines = "ciphertext_updates"

#experiment_numbers = [[4,0]]
experiment_numbers = [[8,0], [8,2], [8,4], [8,8], [8,16],[8,24]]
#experiment_numbers = [[8,8]]
#experiment_numbers = [[0,4],[0,8]]
experimentList = []

for group in experiment_numbers:
  experimentList.append(client_machines + " " + str(group[0]) + "\n" + update_machines + " " + str(group[1]))
print("ExperimentList: " + (str(experimentList)))

logs = []

# " echo $'" + experimentList[0] + "' > arguments.txt; "

def analyze_logs():
  result_to_file = ""
  for log in logs:
    out1 = run_local_task("python3 runExperimentsOnAWS.py aggregate " + log)

    try:
      out2 = run_local_task("python3 analyze_logs.py " + log + "/" + client_machines)
      benchmark1 = out2.decode('utf-8').split("TOTAL")[1].split("Latency:")[1].strip()
    except Exception:
      benchmark1 = "0.0"

    # Try catches for when we have 0 update machines.
    try:
      out3 = run_local_task("python3 analyze_logs.py " + log + "/" + update_machines)
      benchmark2 = out3.decode('utf-8').split("TOTAL")[1].split("Latency:")[1].strip()
    except Exception:
      benchmark2 = "0.0"

    out4 = run_local_task("python3 obtain_statistics.py " + log)
    splits = out4.decode('utf-8').split("\n")
    lastline = splits[len(splits)-2]

    stat = benchmark1 + " " + benchmark2 + "     " + lastline
    print(stat)
    result_to_file += stat + "\n"
  return result_to_file



def run_local_task(cmd):
  p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr = subprocess.STDOUT,
                        shell=True, preexec_fn=os.setsid)
  out, err = p.communicate()
  return out


for i in range(len(experimentList)):
  print("Experiment: __________________")
  print(experimentList[i])
  cmd = " echo $'" + experimentList[i] + "' > experiment_parameters.txt; "
  run_local_task(cmd)


  cmd = "python3 runExperimentsOnAWS.py run"
  out = run_local_task(cmd)
  splits = out.decode("utf-8").split("\n")
  logname = splits[len(splits)-2]
  print(logname)
  logs.append(logname)

final_res = analyze_logs()

run_local_task("echo " + final_res + " > results.txt")
