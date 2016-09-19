#!/bin/bash

if [[ $# != 2 ]]; then
  echo "$0 <amz|ggl|maz> <vmtype>"
  exit 1
fi

PROVIDER=$1
VMTYPE=$2

if [[ $PROVIDER == 'amz' ]]; then
  # STEP 1: create instance
  INSTANCE_ID=`aws ec2 run-instances \
  --image-id ami-2d39803a \
  --security-group-ids sg-13e59177 \
  --subnet-id subnet-3e4faa15 \
  --associate-public-ip-address \
  --count 1 \
  --instance-type $VMTYPE \
  --key-name weijia_nvirginia \
  --query 'Instances[0].InstanceId'|sed 's/"//g'`
  if [[ $? != 0 ]]; then
    echo "Fail to create instance, provider:$PROVIDER VMTYPE:$VMTYPE."
    exit 3
  fi
  aws ec2 create-tags --resources $INSTANCE_ID --tags Key=Name,Value=CloudModel
  # STEP 2: get public ip
  sleep 10 # wait for 10 seconds.
  PUBLIC_IP=`aws ec2 describe-instances \
  --instance-ids $INSTANCE_ID \
  --query 'Reservations[0].Instances[0].PublicIpAddress'|sed 's/"//g'`
  if [[ $? != 0 ]]; then
    echo "Fail to get public ip, provider:$PROVIDER VMTYPE:$VMTYPE INSTANCE_ID:$INSTANCE_ID, terminate..."
    aws ec2 terminate-instances --instance-ids $INSTANCE_ID
    exit 4
  fi
  # STEP 3: echo the result
  echo "$PROVIDER $VMTYPE $INSTANCE_ID $PUBLIC_IP"
  exit 0
else
  echo "Provider:$PROVIDER is not supported yet. please pick one in <amz>."
  exit 2
fi
