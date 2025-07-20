#!/bin/bash -e

current_path=$(pwd)

path_file="path.txt"

work_dir=$current_path/$(cat "$path_file")

server_dir=$current_path/openvpn_server
client_dir=$current_path/openvpn_client

mkdir -p $client_dir

get_data() {
	array=($(grep -n "\-\-\-\-\-" $1 | cut -d: -f1))
	sed -n "${array[0]},${array[1]}p" $1
}

cd "$work_dir"

client_count=0
echo "Client list:"
for item in $(ls -1 "pki/issued/"); do
        _name=$(basename "$item" .crt)
        [ "server" = "$_name" ] && continue
        echo "    $_name"
        client_count=$((client_count + 1))
done
echo "Total: $client_count"

while [ -z "$client_name" ]; do
        echo -n "Name of the client to be make: "
        read client_name
done
ca_content=$(get_data pki/ca.crt)
cert_content=$(get_data pki/issued/$client_name.crt)
key_content=$(get_data pki/private/$client_name.key)
cd - 1> /dev/null

ta_content=$(get_data $server_dir/ta.key)

while [ -z "$remote_addr" ]; do
        echo -n "Remote address: "
        read remote_addr
done


cd $client_dir

cat <<EOF > client_$client_name.ovpn
client
dev tun
proto udp

remote $remote_addr 5666
resolv-retry infinite
nobind
persist-key
persist-tun

cipher AES-256-CBC
auth SHA256
auth-nocache
key-direction 1
comp-lzo no
verb 3

<ca>
$ca_content
</ca>

<cert>
$cert_content
</cert>

<key>
$key_content
</key>

# tls-auth
<tls-auth>
$ta_content
</tls-auth>
EOF

cd - 1> /dev/null

echo "done."
echo "output: $client_dir/client_$client_name.ovpn"

