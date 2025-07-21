#!/bin/bash -e

lan_if=$(ip route list default | awk '{print $5}')
vpn_if=tun0

get_ip_addr() {
	ip -4 addr show $1 | awk '/inet/ {print $2}' | cut -d/ -f1
}

lan_ip=$(get_ip_addr $lan_if)
vpn_ip=$(get_ip_addr $vpn_if)

echo "LAN: $lan_ip ($lan_if)"
echo "VPN: $vpn_ip ($vpn_if)"

./discovery_proxy/discover_proxy $vpn_ip $lan_ip

