#!/bin/bash -e

current_path=$(pwd)

path_file="path.txt"

work_dir=$current_path/$(cat "$path_file")

server_dir=$current_path/openvpn_server

mkdir -p $server_dir

while [ -z "$lan_net" ]; do
        echo -n "LAN network: "
        read lan_net
done

cd $server_dir

[ ! -f "ta.key" ] && openvpn --genkey secret ta.key

cat <<EOF > server.conf
# Protocol and port settings
port 5666
proto udp
dev tun

# Server Certificate and Key
ca $work_dir/pki/ca.crt
dh $work_dir/pki/dh.pem
cert $work_dir/pki/issued/server.crt
key $work_dir/pki/private/server.key
crl-verify $work_dir/pki/crl.pem

# IP address pool and routing settings
server 10.0.0.0 255.255.255.0
push "route $lan_net"
ifconfig-pool-persist $server_dir/ipp.txt

# Other  settings
keepalive 10 120
client-to-client
topology subnet
tls-auth $server_dir/ta.key 0
data-ciphers AES-256-GCM:AES-128-GCM:AES-256-CBC
data-ciphers-fallback AES-256-CBC
auth SHA256
comp-lzo no
user nobody
group nogroup
persist-key
persist-tun
status $server_dir/openvpn-status.log
verb 3
EOF

cd - 1> /dev/null

echo "done."
echo "output: $server_dir/server.conf"

