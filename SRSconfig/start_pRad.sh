#! /bin/bash

cd ${SRS_SLOWCONTROL_DIR}

echo "Configuring FEC ..."
source ./configureFEC.sh

echo "start FEC IP 10.0.0.2"
slow_control/slow_control slow_control/startPRad.txt 