#!/bin/sh
echo "Content-Type: text/plain"
echo ""
cat /proc/net/dev | grep eth0
