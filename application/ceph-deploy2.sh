#!/bin/bash

# Note: Please look at the README and follow the instructions. There are a few things that need to be done
# manually after running ceph-deploy1.sh and before running ceph-deploy2.sh (this script).

# Argument 1: Admin node name
ADMIN_NODE=$1

# Argument 2: Monitor node name
MONITOR_NODE=$2

# Argument 4-n: All of the OSD node names
OSD_NODES=${@:3}

NUM_OSD=$(($# - 2))

for i in `seq 1 $NUM_OSD`;
do
    # TODO: This is not parametrized, it assumed the OSD names are ceph-osd#
    ceph-deploy osd prepare ceph-osd"$i":/var/local/osd"$i"
done

for i in `seq 1 $NUM_OSD`;
do
    # TODO: This is not parametrized, it assumed the OSD names are ceph-osd#
    ceph-deploy osd activate ceph-osd"$i":/var/local/osd"$i"
done

ceph-deploy admin $ADMIN_NODE $MONITOR_NODE $OSD_NODES

sudo chmod +r /etc/ceph/ceph.client.admin.keyring
