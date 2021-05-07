#!/bin/sh

sudo chmod +x ./server
sudo touch /etc/systemd/system/cooldaemon.service
sudo echo "
[Unit]
Description=Cool daemon

[Service]
ExecStart=$PWD/server/server 8080
Restart=on-failure

[Install]
WantedBy=multi-user.target
" > /etc/systemd/system/cooldaemon.service