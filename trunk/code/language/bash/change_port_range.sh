#!/usr/bin/env bash

echo "10000 65535" > /proc/sys/net/ipv4/ip_local_port_range
echo "change local port range"
cat /proc/sys/net/ipv4/ip_local_port_range

