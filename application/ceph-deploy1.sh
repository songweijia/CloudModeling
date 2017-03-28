#!/bin/bash

# Argument 1: monitor node name
MONITOR_NODE=$1

# Argument 2: Private IP Subnet
SUBNET=$2

cd ~
mkdir cluster
cd cluster

ceph-deploy new $MONITOR_NODE

# Modify 'ceph.conf' file with updated properties
# Need these specifically for Fractus instances
echo "osd pool default size = 2" >> ceph.conf
echo "public network = $SUBNET" >> ceph.conf
echo "[osd]" >> ceph.conf
echo "filestore xattr use omap = true" >> ceph.conf

# TODO: Make this parametrized
ceph-deploy install ceph-osd1 ceph-osd2 ceph-osd3 ceph-mon1 ceph-admin

ceph-deploy mon create-initial

