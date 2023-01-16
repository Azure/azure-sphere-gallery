FROM ubuntu:22.04

RUN apt-get update
RUN apt-get install -y dnsutils bind9 net-tools inetutils-ping avahi-utils
RUN apt-get install -y less

ADD db.home /etc/bind/db.home
ADD named.conf.options /etc/bind/named.conf.options
ADD named.default /etc/default/named

ADD entry.sh /root/entry.sh

EXPOSE 53/tcp
EXPOSE 53/udp
EXPOSE 5353/udp

ENTRYPOINT /root/entry.sh