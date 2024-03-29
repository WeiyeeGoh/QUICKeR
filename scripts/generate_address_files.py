import boto3
import botocore
import paramiko
import sys
import threading
import time
import datetime
import os
import subprocess


private_key_pem = "../../.ssh/lawrence-ucsb-test-1.pem"


# def wait_instance_until_resady(instance_id):
#     ec2_c = boto3.client('ec2')
#     instance_state = "";
#     while(instance_state != "enabled"):
#         response = ec2_c.monitor_instances(InstanceIds=[instance_id])
#         instance_state = response['InstanceMonitorings'][0]['Monitoring']['State']
#         print(response['InstanceMonitorings'][0]['Monitoring']['State'])
#         #print(response)
#         time.sleep(1)

def wait_instance_until_resady(instance_id):
    ec2_r = boto3.resource('ec2', region_name="us-east-2")
    instance_state = ""
    while(instance_state != "running"):
        instance = ec2_r.Instance(instance_id)
        instance_state = instance.state['Name']
        print("%s: %s" %(instance_id, instance_state))
        #print(response)
        time.sleep(1)

def wait_instance_until_stopped(instance_id):
    ec2_r = boto3.resource('ec2', region_name="us-east-2")
    instance_state = ""
    while(instance_state != "stopped"):
        instance = ec2_r.Instance(instance_id)
        instance_state = instance.state['Name']
        print("%s: %s" %(instance_id, instance_state))
        #print(response)
        time.sleep(1)


def call_to_instance(ip_address, command, command_type=None, logname=None, test_name=None):
    print("connecting to %s" %(ip_address))

    key = paramiko.RSAKey.from_private_key_file(private_key_pem)
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

    # Connect/ssh to an instance
    # Here 'ubuntu' is user name and 'instance_ip' is public IP of EC2
    client.connect(hostname=ip_address, username="ec2-user", pkey=key)

    # Execute a command(command) after connecting/ssh to an instance
    stdin, stdout, stderr = client.exec_command(command)
    

    if command_type == "setup":
        print(stdout.read())
        #pass
    elif command_type == "start_db":
        pass
    elif command_type == "run":
        pass
    elif command_type == "aggregate":
        # make log directory to aggregate all the log files
        if not os.path.exists(logname):
            os.mkdir(logname)

        path = logname+'/'+test_name
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

def send_file_to_instance(ip_address, filename, remoteFilePath):
    key = paramiko.RSAKey.from_private_key_file(private_key_pem)
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(hostname=ip_address, username="ec2-user", pkey=key)

    ftp_client=client.open_sftp()
    print("GOOD TILL HERE\n")
    ftp_client.put(filename,remoteFilePath)
    ftp_client.close()
    client.close()
 





# Get all ip addresses of clients
all_ip_addresses = []
region = "us-east-2"
ec2 = boto3.resource('ec2', region_name=region)
ec2_client = boto3.client('ec2', region_name=region)



#######GET STOPPED CLIENT MACHINES################
stopped_client_machines = ec2_client.describe_instances(Filters=[
    {
        'Name': 'instance-state-name',
        'Values': ['stopped']
    },
    {
        'Name': 'tag:Client_Type',
        'Values': ['Client_Test']
    }
])
stuff = stopped_client_machines['Reservations']
total =0
stopped_client_list = []
client_priv_ip_addr = []
for res in stuff:
    print(len(res["Instances"]))
    for i in range(len(res["Instances"])):
        print(res["Instances"][i]['InstanceId'])
        print(res["Instances"][i]['PrivateIpAddress'])
        total += 1
        stopped_client_list.append(res["Instances"][i]['InstanceId'])
        client_priv_ip_addr.append(res["Instances"][i]['PrivateIpAddress'])
print("total: %d\n" %(total))

#########Get Stopped DB Machines#######################
stopped_db_machines = ec2_client.describe_instances(Filters=[
    {
        'Name': 'instance-state-name',
        'Values': ['stopped']
    },
    {
        'Name': 'tag:Type',
        'Values': ['redisdb_test']
    }
])
stuff = stopped_db_machines['Reservations']
total =0
stopped_db_list = []
db_priv_ip_addr = []
for res in stuff:
    print(len(res["Instances"]))
    for i in range(len(res["Instances"])):
        print(res["Instances"][i]['InstanceId'])
        print(res["Instances"][i]['PrivateIpAddress'])
        total += 1
        stopped_db_list.append(res["Instances"][i]['InstanceId'])
        db_priv_ip_addr.append(res["Instances"][i]['PrivateIpAddress'])
print("total: %d\n" %(total))


######## STARTING INSTANCE STUFF
#### Start Up all the client instances
print("Starting up all the instances\n")
thread_list = []
for stopped_client in stopped_client_list:
    try:
        response = ec2_client.start_instances(InstanceIds=[stopped_client], DryRun=False)
        #print(response)

        x = threading.Thread(target=wait_instance_until_resady, args=(stopped_client,))
        x.start()
        thread_list.append(x)
    except Exception as e:
        print(e)

#### Start up all the stopped db instances
for stopped_db in stopped_db_list:
    try:
        response = ec2_client.start_instances(InstanceIds=[stopped_db], DryRun=False)
        #print(response)

        x = threading.Thread(target=wait_instance_until_resady, args=(stopped_db,))
        x.start()
        thread_list.append(x)
    except Exception as e:
        print(e)

# Wait for them all to join up
for thread in thread_list:
    thread.join()

started_client_list = stopped_client_list
started_db_list = stopped_db_list
print("All instances are up and running\n")

exit(0)


#### Startup Redis on each of the db (and write to db_address.txt file)
print("Startup database program on db servers\n")
f = open("db_address.txt", "w")
print(db_priv_ip_addr)
for i in range(len(db_priv_ip_addr)):
    private_db_address = db_priv_ip_addr[i]
    print("starting: %s\n", private_db_address)
    call_to_instance(private_db_address, "python3 startup_redis_script.py", "start_db")
    f.write(private_db_address + " " + str(5000) + "\n")
    # if i != len(db_priv_ip_addr) - 1:
    #     f.write("\n")

for i in range(30):
    print("time: %d" %i)
    time.sleep(1)
f.close()
print("Done with database startup\n")



# TODO unhardcode
print("perform setup on client instances\n")
cmd = "cd QUICKeR/scripts; python3 setup_client.py 172.31.14.183"
setup_thread_list = []

for i in range(len(client_priv_ip_addr)):
    ip_address = client_priv_ip_addr[i]
    print(ip_address)
    thread = threading.Thread(target=call_to_instance, args=(ip_address,cmd, "setup"))

    setup_thread_list.append(thread)
    thread.start()

for thread in setup_thread_list:
    thread.join()
print("Setup on Client Instances is DONE\n")


print("Wait 10 seconds\n")
time.sleep(15)


#########Run Commands to Start client and update machines
print("Startup client and update machines program\n")
date = datetime.datetime.now()
date_formatted = date.strftime("%y-%m-%d-%X")
logname = "logs/" + date_formatted + "_logs.txt"
output_logname = "logs/" + date_formatted + "_logs"

f = open("experiment_parameters.txt", "r")
experiment_names = []
experiment_count = []
for line in f.readlines():
    line_split = line.split(" ")
    experiment_names.append(line_split[0])
    experiment_count.append(line_split[1])
f.close()

f = open("arguments.txt", "r")
arg_file = []
for line in f.readlines():
    arg_file.append(line)
f.close()

command_list = []
portnum = 5000
index = 0
for i in range(len(experiment_names)):
    test_name = experiment_names[i]
    test_count = int(experiment_count[i])

    machine_port = 6000
    if (experiment_names[i] == "routine_operations_updatable_encryption"):
        addr_fd = open("client_machines_address.txt", "w")
        machine_port = 6000
    elif (experiment_names[i] == "ciphertext_updates_updatable_encryption"):
        addr_fd = open("update_machines_address.txt", "w")
        machine_port = 7000
    else:
        print("Unknown Experiment?? %s\n", experiment_names[i])

    for j in range(test_count):
        arg_file_content = arg_file[0] + arg_file[1] + str(portnum + index) + "\n" + arg_file[3] + arg_file[4]
        ##print(arg_file_content)
        cmd = " cd QUICKeR/scripts;"
        cmd += " echo $'" + arg_file[0] + arg_file[1] + str(portnum + index) + "\n" + arg_file[3] + arg_file[4] + "' > arguments.txt; "
        cmd += " python3 run_client.py " + test_name + " " + logname
        ##print(cmd)
        command_list.append(cmd)

        addr_fd.write(client_priv_ip_addr[index] + " " + str(machine_port) + "\n")
        index += 1

    addr_fd.close()

print(command_list)
for i in range(len(command_list)):
    cmd = command_list[i]
    ip_address = client_priv_ip_addr[i]
    print(ip_address)
    thread = threading.Thread(target=call_to_instance, args=(ip_address,cmd, "run"))

    setup_thread_list.append(thread)
    thread.start()

for thread in setup_thread_list:
    thread.join()









# ######### STOP all client machines
# # Dry run succeeded, call stop_instances without dryrun
# stopping_thread_list = []
# for client_name in started_client_list:
#     try:
#         response = ec2_client.stop_instances(InstanceIds=[client_name], DryRun=False)
#         x = threading.Thread(target=wait_instance_until_stopped, args=(client_name,))
#         x.start()
#         stopping_thread_list.append(x)
#     except Exception as e:
#         print(e)
# for client_name in started_db_list:
#     try:
#         response = ec2_client.stop_instances(InstanceIds=[client_name], DryRun=False)
#         x = threading.Thread(target=wait_instance_until_stopped, args=(client_name,))
#         x.start()
#         stopping_thread_list.append(x)
#     except Exception as e:
#         print(e)

# for thread in stopping_thread_list:
#     thread.join()
# print("All db and clients have been stopped\n")


# print("aggregate log with %s", output_logname);
######### Stop all db machines

# exit(0)
















######### Populate DB HERE
print("Perform populate db\n")
subprocess.run(["../build/src/main/populate_database_updatable", "arguments.txt", "db_address.txt"])

for client_ip_addr in client_priv_ip_addr:
    send_file_to_instance(client_ip_addr, "Output.txt", "./QUICKeR/scripts/Output.txt")


######### Run Round Coord Here
print("Perform round coordinator\n")
subprocess.run(["../build/src/main/update_round_coordinator", "client_machines_address.txt", "update_machines_address.txt", "db_address.txt"])



aggregate_thread_list = []
print("aggregate log with %s", output_logname)
f = open("experiment_parameters.txt", "r")
command_list = []
all_test_names = []
for line in f.readlines():
    line_split = line.split(" ")
    test_name = line_split[0]
    test_count = int(line_split[1])
    all_test_names.append(test_name)

    cmd = "cd QUICKeR/scripts; cat arguments.txt; cat " + output_logname + ".txt"
    command_list += [(test_name, cmd)] * test_count
for i in range(len(client_priv_ip_addr)):
    ip_address = client_priv_ip_addr[i]
    cmd = command_list[i][1]
    test_name = command_list[i][0]
    print(ip_address)
    thread = threading.Thread(target=call_to_instance, args=(ip_address,cmd, "aggregate", output_logname, test_name))

    aggregate_thread_list.append(thread)
    thread.start()

for thread in aggregate_thread_list: 
    thread.join()


######### STOP all client machines
# Dry run succeeded, call stop_instances without dryrun
stopping_thread_list = []
for client_name in started_client_list:
    try:
        response = ec2_client.stop_instances(InstanceIds=[client_name], DryRun=False)
        x = threading.Thread(target=wait_instance_until_stopped, args=(client_name,))
        x.start()
        stopping_thread_list.append(x)
    except Exception as e:
        print(e)
for client_name in started_db_list:
    try:
        response = ec2_client.stop_instances(InstanceIds=[client_name], DryRun=False)
        x = threading.Thread(target=wait_instance_until_stopped, args=(client_name,))
        x.start()
        stopping_thread_list.append(x)
    except Exception as e:
        print(e)

for thread in stopping_thread_list:
    thread.join()
print("All db and clients have been stopped\n")






print("aggregate log with %s", output_logname)
for i in range(len(all_test_names)):
    print("log directory:  %s", output_logname + "/" + all_test_names[i])
#print("aggregate log with %s", output_logname)


exit(0)



###########REMOVE################
# try:
#     ec2_client.start_instances(InstanceIds=["i-00175a642f594c0b5"], DryRun=True)
# except Exception as e:
#     if 'DryRunOperation' not in str(e):
#         raise
## Dry run succeeded, run start_instances without dryrun







# TODO Populate DB Command here

# ip_address = "3.136.158.29"
#send_file_to_instance(ip_address, "Output.txt", "./QUICKeR/scripts/Output.txt")

# Run Round Coordinator












# for instance in ec2.instances.all():
#     name = ''
#     tags = {}

#     if instance.tags != None:
#         for tag in instance.tags:
#             if tag['Key'] == 'Type' and (tag['Value'] == 'MainCode'):
#                 if instance.public_ip_address != None:
#                     all_ip_addresses.append(instance.public_ip_address)
# all_ip_addresses.sort()
# print(all_ip_addresses)






# for i in range(len(command_list)):

#     ip_address = all_ip_addresses[i]
#     print(ip_address)

#     if sys.argv[1] == "aggregate":
#         cmd = command_list[i][1]
#         test_name = command_list[i][0]
#         thread = threading.Thread(target=call_to_instance, args=(ip_address,cmd,test_name))
#     else:
#         cmd = command_list[i]
#         thread = threading.Thread(target=call_to_instance, args=(ip_address,cmd))

#     thread_list.append(thread)
#     thread.start()







    
# argument_file = ["172.31.7.183\nbob:Applejack2022\n"] 

#     date = datetime.datetime.now()
#     date_formatted = date.strftime("%y-%m-%d-%X")
#     logname = "logs/" + date_formatted + "_logs.txt"
#     output_logname = "logs/" + date_formatted + "_logs"

#     f = open("experiment_parameters.txt", "r")
#     command_list = []
#     portnum = 5000
#     count = 0
#     for line in f.readlines():
#         line_split = line.split(" ")
#         test_name = line_split[0]
#         test_count = int(line_split[1])

#         if (test_count % len(argument_files) != 0):
#             print ("Test counts not divisible by number of redis machines")
#             exit(0)
#         for j in range (len(argument_files)):
#             for k in range (portnum,  int(test_count / len(argument_files)) + portnum):
#                 cmd = " cd QUICKeR/scripts;"
#                 cmd += " echo $'" + argument_files[j] + str(k) +  "' > arguments.txt; "
#                 cmd += " python3 run_client.py " + test_name + " " + logname
            
            
#                 command_list += [cmd]
#         portnum += int(test_count)





# for i in range(len(client_priv_ip_addr)):
#     ip_address = client_priv_ip_addr[i]
#     print(ip_address)
#     thread = threading.Thread(target=call_to_instance, args=(ip_address,cmd, "run"))

#     setup_thread_list.append(thread)
#     thread.start()

# for thread in setup_thread_list:
#     thread.join()
