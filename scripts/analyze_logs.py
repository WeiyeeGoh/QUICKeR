import sys
import os


# Get Path Arg
if len(sys.argv) < 2:
    print("Need path to log directory")
    exit(0)
path = sys.argv[1]

# Get Files in Directory
files = os.listdir(path)

# Read each file and parse data
dictionary_arr = []
latency_arr = []
for file in files:
    latency_value = 0.0
    #retry_dict = {}
    unique_retries = 0
    total_retries = 0

    final_latency_count = 0
    with open(path + "/" + file, "r") as f:
        for line in f:
            line_words = line.split(" ")
            if (len(line_words) < 2):
                continue
            elif line_words[1] == "Retry_Message:":
                unique_id = line_words[2]
                retries = int(line_words[4])
                total_retries += retries
                unique_retries += 1
            elif line_words[1] == "latency_final:":
                latency_value += float(line_words[2])
                final_latency_count += 1

    if final_latency_count > 0:
        latency_value /= final_latency_count

    latency_arr.append(latency_value)
    dictionary_arr.append([unique_retries, total_retries])

total_unique_retries = 0
total_retries = 0
average_latency = 0
failed_machines = 0
for i in range(len(files)):
    print("------------ " + files[i] + " ------------")

    #unique_retries = len(dictionary_arr[i].keys())
    unique_retries = dictionary_arr[i][0]
    print("Number of Unique Retries:    %d" %unique_retries)

    # retries_in_file = 0
    # for key in dictionary_arr[i]:
    #   retries_in_file += dictionary_arr[i][key] 
    retries_in_file = dictionary_arr[i][1]
    print("Total Retries:           %d" %retries_in_file)
    print("Latency:             %f" %(latency_arr[i]))

    if (latency_arr[i] == 0.0):
        failed_machines += 1

    total_unique_retries += unique_retries
    total_retries += retries_in_file

average_latency = sum(latency_arr) / (len(latency_arr) - failed_machines)


print("------------TOTAL------------")
print("Total Number of Machines:    %d" %len(files))
print("Number of Unique Retries:    %d" %total_unique_retries)
print("Total Retries:               %d" %total_retries)
print("Number of Failed Machines:   %d" %failed_machines)
print("Latency:                     %f" %average_latency)





