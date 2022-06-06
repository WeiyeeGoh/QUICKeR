import subprocess
import sys
import os
import time
import datetime
import json

# Old Redis Instance ID
#instance_ids = ["i-0825f06845e5b5491", "i-00c678d55268f6ac1"]

# New KeyDB Instance ID
instance_ids = ["i-09d27e41a8e88eb9c"]

if len(sys.argv) < 2:
    print("Need to have log_file name as parameter")
    exit(0)

def format_command(metric_name, period, stat_name, instance_id, log_time, interval):
    cmd = "aws cloudwatch get-metric-statistics "
    cmd += "--namespace AWS/EC2 "
    cmd += "--metric-name " + str(metric_name) + " "
    cmd += "--period " + str(period) + " "
    cmd += "--statistics " + str(stat_name) + " "
    cmd += "--dimensions "
    cmd += "Name=InstanceId,Value=" + str(instance_id) + " "

    timestamp = log_time.split("/")[1].split("_")[0]
    time1 = datetime.datetime.strptime(timestamp, "%y-%m-%d-%X")

    delta = datetime.timedelta(minutes=int(interval))
    time2 = time1 + delta

    start_time = time1.strftime("%Y-%m-%dT%X")
    end_time = time2.strftime("%Y-%m-%dT%X")

    cmd += "--start-time " + str(start_time) + " --end-time " + str(end_time)
    return cmd


######## Network In ##########
period = 60
print("--------- Network In(mbps) ----------")
for instance_id in instance_ids:
    network_out_cmd = format_command("NetworkIn", period, "Sum", instance_id, sys.argv[1], 7)
    p = subprocess.Popen(network_out_cmd, stdout=subprocess.PIPE,shell=True, preexec_fn=os.setsid)
    out, err = p.communicate()
    results = json.loads(out)
    results["Datapoints"] = sorted(results["Datapoints"], key=lambda x: x["Timestamp"], reverse=False)

    network_in_arr = []
    for i in range(len(results["Datapoints"])):
        mbps_in = float(results["Datapoints"][i]["Sum"]) * 8 / period /1000000
        network_in_arr.append(round(mbps_in, 2))
    print(str(instance_id) + ": " + str(network_in_arr))


######## Network Out ##########
period = 60
print("--------- Network Out(mbps) ----------")
for instance_id in instance_ids:
    network_out_cmd = format_command("NetworkOut", period, "Sum", instance_id, sys.argv[1], 7)
    p = subprocess.Popen(network_out_cmd, stdout=subprocess.PIPE,shell=True, preexec_fn=os.setsid)
    out, err = p.communicate()
    results = json.loads(out)
    results["Datapoints"] = sorted(results["Datapoints"], key=lambda x: x["Timestamp"], reverse=False)

    network_out_arr = []
    for i in range(len(results["Datapoints"])):
        mbps_out = float(results["Datapoints"][i]["Sum"]) * 8 / period / 1000000
        network_out_arr.append(round(mbps_out, 2))
    print(str(instance_id) + ": " + str(network_out_arr))

######## CPUUtilization Average ##########
period = 60
print("--------- CPU Utilization % ----------")
for instance_id in instance_ids:
    network_out_cmd = format_command("CPUUtilization", period, "Average", instance_id, sys.argv[1], 7)
    p = subprocess.Popen(network_out_cmd, stdout=subprocess.PIPE,shell=True, preexec_fn=os.setsid)
    out, err = p.communicate()
    results = json.loads(out)
    results["Datapoints"] = sorted(results["Datapoints"], key=lambda x: x["Timestamp"], reverse=False)

    cpu_util_arr = []
    for i in range(len(results["Datapoints"])):
        cpu_util = float(results["Datapoints"][i]["Average"])
        cpu_util_arr.append(round(cpu_util, 2))
    print(str(instance_id) + ": " + str(cpu_util_arr))


print(str(network_in_arr[2]) + " " + str(network_out_arr[2]) + "  " + str(cpu_util_arr[2]))