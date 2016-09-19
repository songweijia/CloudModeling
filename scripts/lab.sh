# this command created an instance, and the output the id of the instance.
aws ec2 run-instances \
--image-id ami-2d39803a \
--security-group-ids sg-13e59177 \
--subnet-id subnet-3e4faa15 \
--associate-public-ip-address \
--count 1 \
--instance-type t2.micro \
--key-name weijia_nvirginia \
--query 'Instances[0].InstanceId'

# this command query the public ip address of the new created instance
aws ec2 describe-instances \
--instance-ids <i-0763af98> \
--query 'Reservations[0].Instances[0].PublicIpAddress'

# this command connect to the instance without checking
ssh -o StrictHostKeyChecking=no -i weijia_nvirginia.pem ubuntu@<ipaddress>

# these commands install streams
sudo apt-get update
sudo apt-get install gcc fort77 -y

# these command terminate the instance
aws ec2 terminate-instances --instance-ids i-0763af98
