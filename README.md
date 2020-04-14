# Se.C.O.Li Consular Data daemon

Consular Data daemon for the [Fast It Project](https://serviziconsolari.esteri.it/) (formerly Se.C.O.Li Project) by the Ministry of Foreign Affairs of Italy.

I was involved in the project in the years 2013-14.

In order to connect to the daemon please use the script `SCEd-client`.

## Install

In CentOS 8, just install the prerequisites

```
dnf install libuuid libuuid-devel
dnf install glib2 glib2-devel
dnf install log4c log4c-devel
dnf install sqlite sqlite-devel
dnf install jansson jansson-devel
dnf install libconfig
dnf install lua
dnf install libdb
dnf install libevent
dnf --enablerepo=PowerTools install libconfig-devel
```

## Examples of usage

```
telnet localhost 5005

HELLO
SCE GATEWAY VERSION 0.3.3 PROTOCOL VERSION 0

FIND SPECIMINA OF ZIPCODES
235
```
