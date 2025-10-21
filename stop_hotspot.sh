#!/bin/bash
# Script tắt WiFi Hotspot

IFACE="wlp1s0"  # đổi nếu card WiFi khác tên
echo "👉 Tắt hotspot trên $IFACE ..."
nmcli connection down Hotspot

