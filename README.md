# Hoozz Play Server For Linux

## Ubuntu version
```shell
$ cat /proc/version
Linux version 6.8.0-64-generic (buildd@lcy02-amd64-101) (x86_64-linux-gnu-gcc-12 (Ubuntu 12.3.0-1ubuntu1~22.04) 12.3.0, GNU ld (GNU Binutils for Ubuntu) 2.38) #67~22.04.1-Ubuntu SMP PREEMPT_DYNAMIC Tue Jun 24 15:19:46 UTC 2
```

## Dependencies
```shell
sudo apt install openvpn easy-rsa
```

## hoozz_play_ca_manager usage
```shell
$ ./hoozz_play_ca_manager.sh
@@@@@@@@@@@@@@ Hoozz Play CA Manager @@@@@@@@@@@@@@@
------ Copyright (c) 2025 Hoozz (huxiangjs) --------
Usage:
    -h                   Show help
    -i                   Initialize workspace
    -c [dir]             Clear workspace
    -s                   Show all certificate information
    -a                   Issue a certificate to the client
    -r                   Revoking a client certificate


$ ./hoozz_play_ca_manager.sh -i
Working directory(default hoozz-play-ca):                  // <----------- Change to your directory
...
Enter New CA Key Passphrase:                               // <----------- CA certificate password
Re-Enter New CA Key Passphrase:                            // <----------- Repeat
...
Common Name (eg: your user, host, or server name) [Easy-RSA CA]:openvpn   // <------------ Set to the name you want
...
/home/huxiang/Desktop/hoozz_play_linux_server/hoozz-play-ca/pki/ca.crt
...
Enter pass phrase for ... pki/private/ca.key:              // <----------- CA certificate password
...
DH parameters of size 2048 created at /home/huxiang/Desktop/hoozz_play_linux_server/hoozz-play-ca/pki/dh.pem


$ ./hoozz_play_ca_manager.sh -a
Client Name: phone_oppo_k11                                // <----------- Set to the name you want
...
Enter pass phrase for ... pki/private/ca.key:              // <----------- CA certificate password

```

## make_openvpn_server_cfg usage
```shell
$ ./make_openvpn_server_cfg.sh
LAN network: 192.168.31.0 255.255.255.0                    // <----------- Set to your LAN (where the device is located)
done.
output: ... openvpn_server/server.conf
```

## make_openvpn_client_ovpn usage
```shell
$ ./make_openvpn_client_ovpn.sh
Client list:
    phone_oppo_k11                                         // <----------- You generated earlier
Total: 1
Name of the client to be make: phone_oppo_k11
Remote address: xxx.xxx.xxx.xxx                            // <----------- Your IP address or domain name
done.
output: ... openvpn_client/client_phone_oppo_k11.ovpn
```

## ubuntu_config_nat usage
* Translate traffic on tun0 network card
```shell
sudo ./ubuntu_config_nat.sh
```

## discovery_proxy usage
* Since the tun network card does not support broadcast, it is necessary to add a proxy to the discovery protocol
```shell
$ cd discovery_proxy/

$ make
gcc -O2 -Wall -c main.c -o main.o -lpthread
gcc -O2 -Wall main.o -o discover_proxy

$ cd ..

$ ./discovery_proxy.sh
LAN: 192.168.31.250 (enp0s25)
VPN: 10.0.0.1 (tun0)
@@@@@@@@@@@@@@ Discover Proxy @@@@@@@@@@@@@@
...
```

## Useful Tips
* Start OpenVPN: `openvpn --config openvpn_server/server.conf`
* When you use `hoozz_play_ca_manager.sh -r xxx` to revoke the certificate, you need to restart OpenVPN to take effect
* Your client cfg file: `openvpn_client/client_phone_oppo_k11.ovpn`

