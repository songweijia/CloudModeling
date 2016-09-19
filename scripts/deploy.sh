#!/bin/bash
if [[ $# != 5 ]]; then
  echo "USAGE $0 <host/ip> <user_name> <key_name> <cloud_provider> <vm_type>"
  exit 1
fi

# update local knowhosts
ssh -o StrictHostKeyChecking=no -i $KEYDIR/$KEYNAME $USERNAME@$HOST pwd

KEYDIR=../cred
HOST=$1
USERNAME=$2
KEYNAME=$3
WORKDIR=`ssh -o StrictHostKeyChecking=no -i $KEYDIR/$KEYNAME $USERNAME@$HOST pwd`
PROVIDER=$4
VMTYPE=$5
VCPUNUM=`python getvmconfig.py $PROVIDER $VMTYPE cpu`
MEMSIZE=`python getvmconfig.py $PROVIDER $VMTYPE ram`

#################################
# install make and gcc
#################################
ssh -o StrictHostKeyChecking=no -i $KEYDIR/$KEYNAME $USERNAME@$HOST sudo apt-get update
ssh -o StrictHostKeyChecking=no -i $KEYDIR/$KEYNAME $USERNAME@$HOST sudo apt-get install make gcc fort77 -y

#################################
# install rr
#################################
RR_PKG=rr.tar.bz2
# 1) upload
scp -o StrictHostKeyChecking=no -i $KEYDIR/$KEYNAME $RR_PKG $USERNAME@$HOST:$WORKDIR
ssh -o StrictHostKeyChecking=no -i $KEYDIR/$KEYNAME $USERNAME@$HOST tar -xf $RR_PKG

#################################
# install stream
#################################
STREAM_PKG=stream.tar.bz2
#) upload
scp -o StrictHostKeyChecking=no -i $KEYDIR/$KEYNAME $STREAM_PKG $USERNAME@$HOST:$WORKDIR
ssh -o StrictHostKeyChecking=no -i $KEYDIR/$KEYNAME $USERNAME@$HOST tar -xf $STREAM_PKG
ssh -o StrictHostKeyChecking=no -i $KEYDIR/$KEYNAME $USERNAME@$HOST "cd $WORKDIR/stream;make clean;make"

#################################
# deploy crontab
#################################
cat deploy.cron \
 | sed "s:\[rrdir\]:$WORKDIR\/rr:g" \
 | sed "s:\[streamdir\]:$WORKDIR\/stream:g" \
 | sed "s:\[vcpunum\]:$VCPUNUM:g" \
 > tmp/deploy.cron.$HOST
scp -o StrictHostKeyChecking=no -i $KEYDIR/$KEYNAME tmp/deploy.cron.$HOST $USERNAME@$HOST:$WORKDIR
ssh -o StrictHostKeyChecking=no -i $KEYDIR/$KEYNAME $USERNAME@$HOST crontab deploy.cron.$HOST
