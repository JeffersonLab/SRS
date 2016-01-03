#! /bin/bash

cd config

echo "======== CONFIGURATION OF SRS FEC IP 10.0.1.2"
echo ""
echo "set 10.0.1.2 -> 10.0.0.3"
slow_control  set_IP10012.txt
usleep 100000
echo "ADC_10012 config"
slow_control  adc_IP10012.txt
usleep 100000
echo "FEC_10012 config"
slow_control  fecCalPulse_IP10012.txt
usleep 100000
echo "APV_10012 config"
slow_control  apv_IP10012.txt
usleep 100000
echo "APVreset_10012 config"
slow_control  fecAPVreset_IP10012.txt
usleep 100000
echo "PLL_10012 config"
slow_control  pll_IP10012.txt
echo ""
echo "======== CONFIGURATION OF SRS FEC IP 10.0.1.2"
