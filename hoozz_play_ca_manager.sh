#!/bin/bash -e

path_file="path.txt"

if [ -f "$path_file" ]; then
	work_dir=$(cat "$path_file")
else
	work_dir="hoozz-play-ca"
fi

show_help() {
	echo "@@@@@@@@@@@@@@ Hoozz Play CA Manager @@@@@@@@@@@@@@@"
	echo "------ Copyright (c) 2025 Hoozz (huxiangjs) --------"
	echo "Usage: "
	echo "    -h                   Show help"
	echo "    -i                   Initialize workspace"
	echo "    -c [dir]             Clear workspace"
	echo "    -s                   Show all certificate information"
	echo "    -a                   Issue a certificate to the client"
	echo "    -r                   Revoking a client certificate"
}

work_init() {
	echo -n "Working directory(default $work_dir): "
	read work_dir_i
	[ -n "$work_dir_i" ] && work_dir="$work_dir_i"

	if [ -d "$work_dir" ]; then
		echo "ERROR: The working directory already exists"
		echo "Please delete first: $0 -c $work_dir"
		exit 1
	fi

	make-cadir "$work_dir"
	cd "$work_dir"
	./easyrsa init-pki
	./easyrsa build-ca
	./easyrsa build-server-full server nopass
	./easyrsa gen-dh
	cd - 1> /dev/null

	echo "$work_dir" > "$path_file"
}

work_clear() {
	[ -n "$2" ] && work_dir="$work_dir_i"
	echo "WARN: Deleting $work_dir"
	rm -rfI "$work_dir" "$path_file"
}

ca_show() {
	cd "$work_dir"
	# cat pki/index.txt
	for item in $(ls -1 "pki/issued/"); do
		local name=$(basename "$item" .crt)
		[ "server" = "$name" ] && continue
		./easyrsa show-cert "$name"
	done
	cd - 1> /dev/null
}

ca_issuance() {
	cd "$work_dir"
	while [ -z "$client_name" ]; do
		echo -n "Client Name: "
		read client_name
	done
	./easyrsa build-client-full "$client_name" nopass
	cd - 1> /dev/null
}

ca_revoke() {
	cd "$work_dir"

	local count=0
	echo "Client list:"
	for item in $(ls -1 "pki/issued/"); do
		local name=$(basename "$item" .crt)
		[ "server" = "$name" ] && continue
		echo "    $name"
		count=$((count + 1))
	done
	echo "Total: $count"

	while [ -z "$client_name" ]; do
		echo -n "Name of the client to be revoked: "
		read client_name
	done
	./easyrsa revoke "$client_name"
	./easyrsa gen-crl
	cd - 1> /dev/null
}

# Select Menu
if [ "-h" = "$1" ] || [ -z "$1" ]; then
	show_help
elif [ "-i" = "$1" ]; then
	work_init
elif [ "-c" = "$1" ]; then
	work_clear
elif [ "-s" = "$1" ]; then
	ca_show
elif [ "-a" = "$1" ]; then
	ca_issuance
elif [ "-r" = "$1" ]; then
	ca_revoke
else
	echo "ERROR: Unknown parameters '$1'"
	show_help
	exit 1
fi

