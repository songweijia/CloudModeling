#!/bin/bash

while read line; do
  id=`echo $line | awk '{print $3}'`
  aws ec2 terminate-instances --instance-ids $id
done < vm.lst

echo "Don't forget to remove vm.lst after successful clean-up."
