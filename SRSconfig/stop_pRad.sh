#! /bin/bash

cd ${SRS_SLOWCONTROL_DIR}

echo "stop FEC IP 10.0.0.2"
slow_control/slow_control slow_control/stopPRad.txt 