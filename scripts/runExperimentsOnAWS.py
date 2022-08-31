import boto3
import botocore
import paramiko
import sys
import threading
import time
import datetime
import os

# OLD REDIS MACHINE
#argument_files = ["172.31.6.123\nbob:Applejack2022", "172.31.8.137\nbob:Applejack2022"]
#argument_files = ["172.31.10.249\nbob:Applejack2022"]

# NEW KEYDB MACHINE
argument_files = ["172.31.7.183\nbob:Applejack2022\n"] 
private_key_pem = "../../.ssh/lawrence-ucsb-test-1.pem"

activated_client_indexes = None

if len(sys.argv) < 2:
    print("Need argument")
    exit(0)

# Get all ip addresses of clients
all_ip_addresses = []
region = "us-east-2"
ec2 = boto3.resource('ec2', region_name=region)

for instance in ec2.instances.all():
    name = ''
    tags = {}

    if instance.tags != None:
        for tag in instance.tags:
            if tag['Key'] == 'Type' and (tag['Value'] == 'Client'):
                if instance.public_ip_address != None:
                    all_ip_addresses.append(instance.public_ip_address)
all_ip_addresses.sort()
print(all_ip_addresses)


# Set up the command_list
output_logname = ""
if sys.argv[1] == "pull":
    cmd = "cd QUICKeR; git reset --hard HEAD; git checkout main; git reset --hard HEAD; git pull origin main"
    command_list = [cmd] * len(all_ip_addresses)
elif sys.argv[1] == "run":
    date = datetime.datetime.now()
    date_formatted = date.strftime("%y-%m-%d-%X")
    logname = "logs/" + date_formatted + "_logs.txt"
    output_logname = "logs/" + date_formatted + "_logs"

    f = open("experiment_parameters.txt", "r")
    command_list = []
    portnum = 5000
    count = 0
    for line in f.readlines():
        line_split = line.split(" ")
        test_name = line_split[0]
        test_count = int(line_split[1])

        if (test_count % len(argument_files) != 0):
            print ("Test counts not divisible by number of redis machines")
            exit(0)
        for j in range (len(argument_files)):
            for k in range (portnum,  int(test_count / len(argument_files)) + portnum):
                cmd = " cd QUICKeR/scripts;"
                cmd += " echo $'" + argument_files[j] + str(k) +  "' > arguments.txt; "
                cmd += " python3 run_client.py " + test_name + " " + logname
            
            
                command_list += [cmd]
        portnum += int(test_count)

    f.close()
    print("Log is stored at %s" %logname)
elif sys.argv[1] == "setup":
    if len(sys.argv) < 3:
        print("Error: Need HSM ip address")
        exit(0)

    cmd = "cd QUICKeR/scripts; python3 setup_client.py " + sys.argv[2]
    command_list = [cmd] * len(all_ip_addresses)

elif sys.argv[1] == "aggregate":
    if len(sys.argv) < 3:
        print("Error: Need Log Directory Path")
        exit(0)

    f = open("experiment_parameters.txt", "r")
    command_list = []
    for line in f.readlines():
        line_split = line.split(" ")
        test_name = line_split[0]
        test_count = int(line_split[1])

        cmd = "cd QUICKeR/scripts; cat arguments.txt; cat " + sys.argv[2] + ".txt"
        command_list += [(test_name, cmd)] * test_count
    f.close()
else:
    print("Dont recognize this command: %s" %(sys.argv[1]))
    print("USE (1) pull   (2) run   (3) setup <hsm_ip_address>   (4) aggregate <log_dir_path>")
    exit(0)

print(command_list)


# print(all_ip_addresses)

# activated_clients = []
# for i in range(len(all_ip_addresses)):
#   if str(i) in activated_client_indexes:
#       activated_clients.append(all_ip_addresses[i])

# print(activated_clients)
# all_ip_addresses = activated_clients

def call_to_instance(ip_address, command, test_name=None):
    print("connecting to %s" %(ip_address))

    key = paramiko.RSAKey.from_private_key_file(private_key_pem)
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

    # Connect/ssh to an instance
    # Here 'ubuntu' is user name and 'instance_ip' is public IP of EC2
    client.connect(hostname=ip_address, username="ec2-user", pkey=key)

    # Execute a command(command) after connecting/ssh to an instance
    stdin, stdout, stderr = client.exec_command(command)

    if sys.argv[1] == "aggregate":
        # make log directory to aggregate all the log files
        if not os.path.exists(sys.argv[2]):
            os.mkdir(sys.argv[2])

        path = sys.argv[2]+'/'+test_name
        if not os.path.exists(path):
            os.mkdir(path)

        f = open(path + "/" + ip_address + "_log.txt", "w")
        log_output = stdout.read().decode('ascii').strip("\n")
        f.write(log_output)
        f.close()
    else:
        print(stdout.read())
    
    # close the client connection once the job is done
    client.close()

    del client, stdin, stdout, stderr


print(all_ip_addresses)
print(command_list)

print(len(all_ip_addresses))
print(len(command_list))
thread_list = []
for i in range(len(command_list)):

    ip_address = all_ip_addresses[i]
    print(ip_address)

    if sys.argv[1] == "aggregate":
        cmd = command_list[i][1]
        test_name = command_list[i][0]
        thread = threading.Thread(target=call_to_instance, args=(ip_address,cmd,test_name))
    else:
        cmd = command_list[i]
        thread = threading.Thread(target=call_to_instance, args=(ip_address,cmd))

    thread_list.append(thread)
    thread.start()


for thread in thread_list:
    thread.join()

print("DONE WITH TASK")
print(output_logname)

