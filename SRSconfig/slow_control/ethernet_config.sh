!/bin/sh
#==============================================================
# Communication with the FEC card (IP addr 10.0.0.2)
#==============================================================
/etc/init.d/iptables stop
/sbin/ifconfig eth2 10.0.0.3 netmask 255.255.255.0 mtu 9000
/sbin/ifconfig
#==============================================================
