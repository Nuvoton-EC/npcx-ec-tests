#!/bin/bash


echo "board name: '$1' "

echo "app name: '$2' "

if [ -f "app/$2/boards/$1.overlay" ]; then
	export BOARD=$1
	west build -p always app/$2
else
	echo "Skipping build for app '$2' as overlay file '$1.overlay' does not exist."
fi

