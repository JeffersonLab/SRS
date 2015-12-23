#! /bin/bash

COMMAND=bryan

cd ${SRS_SLOWCONTROL_DIR}/slow_control

echo "======== CONFIGURATION OF SRS CARD 0"
echo ""
echo "set 10.0.0.2 -> 10.0.0.3"
$COMMAND ipSet.txt
usleep 100000
#
echo "ADC config"
$COMMAND adc.txt
usleep 100000
#
echo "FEC config for data run"
$COMMAND fecExtTrigger_IP10002NwFrmKondo041715.txt

usleep 100000
#
echo "reset APV"
$COMMAND apvReset.txt
usleep 100000
#
echo "APV config"
$COMMAND apv.txt
usleep 100000
#
echo "PLL config"
$COMMAND pll.txt
echo ""
