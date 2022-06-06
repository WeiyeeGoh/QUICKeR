import subprocess
import sys
import os
import time
import datetime


logs = [
"log_filename"
]


client_machines = "routine_operations"
update_machines = "ciphertext_updates"

# " echo $'" + experimentList[0] + "' > arguments.txt; "


def run_local_task(cmd):
  p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr = subprocess.STDOUT,
                        shell=True, preexec_fn=os.setsid)
  out, err = p.communicate()
  return out


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
    result_to_file += stat #+ "\n"
  return result_to_file

final_res = analyze_logs()

run_local_task("cat " + final_res + " > results.txt")
