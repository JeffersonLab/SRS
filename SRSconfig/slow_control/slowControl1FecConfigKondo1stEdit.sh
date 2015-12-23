#! /bin/bash
echo "======== CONFIGURATION OF SRS CARD 0"
echo ""
echo "set 10.0.0.2 -> 10.0.0.3"
/home/mitra/DATEInstallation/slow_control/slow_control /home/mitra/DATEInstallation/slow_control/ipSet.txt
usleep 100000
#
echo "ADC config"
/home/mitra/DATEInstallation/slow_control/slow_control /home/mitra/DATEInstallation/slow_control/adc.txt
usleep 100000
#
echo "FEC config for data run"
#/home/mitra/DATEInstallation/slow_control/slow_control /home/mitra/DATEInstallation/slow_control/fec_6TimeSample.txt 
#/home/mitra/DATEInstallation/slow_control/slow_control /home/mitra/DATEInstallation/slow_control/fec_3TimeSample.txt 
/home/mitra/DATEInstallation/slow_control/slow_control /home/mitra/DATEInstallation/slow_control/fec_12TimeSample.txt 
#/home/mitra/DATEInstallation/slow_control/slow_control /home/mitra/DATEInstallation/slow_control/fec_test.txt 
#echo "FEC config for test pulse run"
#/home/mitra/DATEInstallation/slow_control/slow_control /home/mitra/DATEInstallation/slow_control/fecTestPulse.txt
usleep 100000
#
echo "reset APV"
/home/mitra/DATEInstallation/slow_control/slow_control /home/mitra/DATEInstallation/slow_control/apvReset.txt
usleep 100000
#
echo "APV config"
/home/mitra/DATEInstallation/slow_control/slow_control /home/mitra/DATEInstallation/slow_control/apv.txt
usleep 100000
#
echo "PLL config"
/home/mitra/DATEInstallation/slow_control/slow_control /home/mitra/DATEInstallation/slow_control/pll.txt
#/home/mitra/DATEInstallation/slow_control/slow_control /home/mitra/DATEInstallation/slow_control/pll2.txt
echo ""