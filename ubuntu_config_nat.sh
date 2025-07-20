#!/bin/bash -e

echo 1 > /proc/sys/net/ipv4/ip_forward

iptables -P FORWARD ACCEPT

public_if=$(ip route list default | awk '{print $5}')

sudo iptables -t nat -A POSTROUTING -o $public_if -j MASQUERADE

sudo iptables -A FORWARD -i tun0 -o $public_if -j ACCEPT
sudo iptables -A FORWARD -i $public_if -o tun0 -m state --state RELATED,ESTABLISHED -j ACCEPT

echo "done."

