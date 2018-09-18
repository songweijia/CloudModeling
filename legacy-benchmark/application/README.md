CloudModel Application Benchmarks
============
The application benchmarks will be performed using Ceph. The existing system will use a configuration with a single `ceph-admin` node, a single `ceph-mon1` monitor node, and 3 `ceph-osd{#}` nodes (i.e. `ceph-osd1`, `ceph-osd2`, and `ceph-osd3`). Email ssh88@cornell.edu for any questions.

Fractus Installation (Ubuntu 14.04)
------------
Five instances need to be created, referring to the admin node, monitor node, and the three OSD nodes. These were provisioned directly through Fractus with a storage size of 20GB (for each node). The installation will be modeled after [this tutorial](http://docs.ceph.com/docs/master/start/quick-start-preflight/) and [this tutorial](https://www.howtoforge.com/tutorial/how-to-install-a-ceph-cluster-on-ubuntu-16-04/). The installation script provided in this repository will assume all of the tasks on the preflight checklist have been finished. Some tips:

1. For the file in `/etc/hosts` used to give hostnames to the other nodes, ensure that you are using the private IP (not the public IP on Fractus). Make sure you add five entries here, one for each of the five nodes listed above. 
2. `sudo` commands will contain an error message due to a hostname that can't be resolved. To fix this, change the entry in `/etc/hostname` to be the name given to the host (such as `ceph-admin`), make sure the the hosts file is updated as mentioned in the previous tip, and use the command `sudo hostname <name>`, where name would be the hostname to use, such as `ceph-admin`.
3. Make users for all the nodes, and use the same username for all the nodes as specified in the second tutorial.
4. Both steps mention using `ssh-copy-id` to copy the `ceph-admin` node's public key to the `authorized_keys` file in all of the other nodes. You will need to enable password authentication on SSH if you need this to work. Otherwise, you can manually copy the public key to the corresponding `~/.ssh/authorized_keys` file on the other 4 nodes. Remember to do this on the `cephuser` account!
5. I followed the firewall instructions from the second tutorial.

Once all the corresponding settings are adhered to, and the nodes can all communicate with one another, run the commands on `ceph-deploy1.sh` on the `ceph-admin` node. It may be better to individually run the commands than to run the script entirely.

After running `ceph-deploy1.sh`, we will need to run a few commands on the OSD nodes. Log onto them and run the following commands (mentioned in the tutorial):
`sudo mkdir /var/local/osd#`
 where `#` represents the OSD number. Make sure the relevant folders are deleted from the OSD nodes if you have done this in the past, as old data may be present.

For each of these OSD nodes, run the following command:
`sudo chown ceph:ceph /var/local/osd#`
where `#` is as before.

Next, go to the `cluster` directory and run the `ceph-deploy2.sh` script. Again, it may be ideal if the commands are run independently rather than on a script.

After these are run, make sure you run the following command on each of the OSD nodes AND the monitor node:
`sudo chmod +r /etc/ceph/ceph.client.admin.keyring`

AWS Installation (Ubuntu 14.04)
------------
For an AWS installation, the same steps as before can be followed. However, keep the following in mind, which have to be configured manually through the AWS web client:

1. Enable a public IP, so you can SSH into the instance
2. Make sure to make the security group configurable to be open to all. To test that this works, for example, you can try pinging `ceph-mon1` from `ceph-admin`. This may not work in the default configuration, even if `ceph-admin` can SSH into `ceph-mon1`.
