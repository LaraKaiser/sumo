#!/bin/bash
$SUMO_HOME/tools/game/DRT/randomRides.py -n osm.net.xml -a drtstops.xml -o rides.xml  --poi-output drt.pois.xml --prefix a -b 0   -e 1600 -p 80 --poi-offset 32 --initial-duration 3
$SUMO_HOME/tools/game/DRT/randomRides.py -n osm.net.xml -a drtstops.xml -o rides2.xml --poi-output drt.pois.xml --prefix a -b 0   -e 1600 -p 60  --poi-offset 32 --initial-duration 3
$SUMO_HOME/tools/game/DRT/randomRides.py -n osm.net.xml -a drtstops.xml -o rides3.xml --poi-output drt.pois.xml --prefix b -b 600 -e 1600 -p 80  --poi-offset 32 --initial-duration 3
