#!/bin/bash

#TODO: Parameterize further
ceph-deploy osd prepare ceph-osd1:/var/local/osd1 ceph-osd2:/var/local/osd2

ceph-deploy osd activate ceph-osd1:/var/local/osd1 ceph-osd2:/var/local/osd2

ceph-deploy admin ceph-admin ceph-mon1 ceph-osd1 ceph-osd2 ceph-osd3

sudo chmod +r /etc/ceph/ceph.client.admin.keyring

