#!/usr/bin/bash
SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"

echo "\n------------\nSETTING TIMEZONE TO CENTRAL US\n------------\n"

sudo cp $SCRIPT_DIR/localtime /etc/localtime
sudo cp $SCRIPT_DIR/timezone /etc/timezone

echo "TIMEZONE SET"

echo "\n------------\nCHANGING USER pi PASSWORD TO carwash1\n------------\n"
sudo passwd pi <<EOF
raspberry
carwash1
carwash1
EOF

echo "\nPASSWORD SET"


echo "\n------------\nSETTING MESSAGE OF THE DAY\n------------\n"

sudo cp $SCRIPT_DIR/motd /etc/motd

echo "\n------------\nUPGRADING RASPBERRY PI\n------------\n"

sudo apt-get update
sudo apt-get upgrade -y

echo "\nUPGRADE COMPLETE"

echo "\n------------\nINSTALLING REQUIRED PROGRAMS\n------------\n"

sudo apt-get install -y wiringpi postgresql-9.6 libpq-dev libpqxx-4.0v5 libpqxx-dev libncurses5-dev git

echo "\nINSTALLATION COMPLETE"

echo "\n------------\nCONFIGURING INSTALLATIONS\n------------\n"

echo "SETTING UP POSTGRESQL"

sudo sed -i "s/local   all             all                                     peer/local   all             all                                     md5/g" /etc/postgresql/9.6/main/pg_hba.conf

echo "\nDONE"

sudo service postgresql restart

echo "\n\nCREATE CARWASH DATABASE"

sudo su postgres -c "psql -c \"CREATE USER washman WITH LOGIN PASSWORD 'cotton';\""
sudo su postgres -c "psql -c \"CREATE DATABASE carwash WITH OWNER washman;\""

echo "\nDONE"
