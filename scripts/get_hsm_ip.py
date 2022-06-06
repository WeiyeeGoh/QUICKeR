import boto3

client = boto3.client('cloudhsmv2')
response = client.describe_clusters()
print(response['Clusters'][0]["Hsms"][0]["EniIp"])