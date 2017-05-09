#!/bin/bash

# Argument 1: Admin node name
ADMIN_NODE=$1

# Argument 2: Monitor node name
MONITOR_NODE=$2

# Argument 3: Private IP Subnet
SUBNET=$3

# Argument 4-n: All of the OSD node names
OSD_NODES=${@:4}

# OSD Pool Default Size:
POOL_SIZE=2

# echo "Summary:\n\nAdmin: $ADMIN_NODE\nMonitor: $MONITOR_NODE\nSubnet: $SUBNET\nOSD Node List: $OSD_NODES\n\n"

cd ~
mkdir cluster
cd cluster

ceph-deploy new $MONITOR_NODE

# Modify 'ceph.conf' file with updated properties
# Need these specifically for Fractus instances
echo "osd pool default size = $POOL_SIZE" >> ceph.conf
echo "public network = $SUBNET" >> ceph.conf
echo "[osd]" >> ceph.conf
echo "filestore xattr use omap = true" >> ceph.conf

ceph-deploy install $OSD_NODES $MONITOR_NODE $ADMIN_NODE 

ceph-deploy mon create-initial
