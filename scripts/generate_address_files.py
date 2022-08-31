import boto3
import botocore
import paramiko
import sys
import threading
import time
import datetime
import os


private_key_pem = "~/.ssh/lawrence-ucsb-test-1.pem"



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
stuff = response_one['Reservations']
total =0
stopped_client_list = []
for res in stuff:
    print(len(res["Instances"]))
    for i in range(len(res["Instances"])):
        print(res["Instances"][i]['InstanceId'])
        total += 1
        stopped_client_list.append(res["Instances"][i]['InstanceId'])
print("total: %d\n" %(total))

#########Get Stopped DB Machines#######################
stopped_db_machines = ec2_client.describe_instances(Filters=[
    {
        'Name': 'instance-state-name',
        'Values': ['stopped']
    },
    {
        'Name': 'tag:Type',
        'Values': ['new_redisdb_test']
    }
])
stuff = response_one['Reservations']
total =0
stopped_db_list = []
for res in stuff:
    print(len(res["Instances"]))
    for i in range(len(res["Instances"])):
        print(res["Instances"][i]['InstanceId'])
        total += 1
        stopped_client_list.append(res["Instances"][i]['InstanceId'])
print("total: %d\n" %(total))






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





exit(0);


def wait_instance_until_resady(instance_id):
    ec2_c = boto3.client('ec2')
    instance_state = "";
    while(instance_state != "enabled"):
        response = ec2_c.monitor_instances(InstanceIds=[instance_id])
        instance_state = response['InstanceMonitorings'][0]['Monitoring']['State']
        print(response['InstanceMonitorings'][0]['Monitoring']['State'])
        #print(response)
        time.sleep(1)

def call_to_instance(ip_address, command):
    print("connecting to %s" %(ip_address))

    key = paramiko.RSAKey.from_private_key_file(private_key_pem)
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

    # Connect/ssh to an instance
    # Here 'ubuntu' is user name and 'instance_ip' is public IP of EC2
    client.connect(hostname=ip_address, username="ec2-user", pkey=key)

    # Execute a command(command) after connecting/ssh to an instance
    stdin, stdout, stderr = client.exec_command(command)
    
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
 

###########REMOVE################
# try:
#     ec2_client.start_instances(InstanceIds=["i-00175a642f594c0b5"], DryRun=True)
# except ClientError as e:
#     if 'DryRunOperation' not in str(e):
#         raise
## Dry run succeeded, run start_instances without dryrun





# thread_list = []
# for stopped_client in stopped_client_list:
#     try:
#         response = ec2_client.start_instances(InstanceIds=[stopped_client], DryRun=False)
#         print(response)

#         x = threading.Thread(target=wait_instance_until_resady, args=(stopped_client,))
#         x.start()
#         thread_list.append(x)
#     except ClientError as e:
#         print(e)

# for thread in thread_list:
#     thread.join()


#call_to_instance("3.136.158.29", "python3 startup_redis_script.py")


# TODO Populate DB Command here

# ip_address = "3.136.158.29"
# send_file_to_instance(ip_address, "Output.txt", "./QUICKeR/scripts/Output.txt")

# Run Round Coordinator