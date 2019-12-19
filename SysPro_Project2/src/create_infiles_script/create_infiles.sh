#!/bin/bash

string="1234567890AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz"
length=${#string}
if [ ! -e "fill_file" ]; then
	gcc -o fill_file fill_file.c
fi
fillfile="$(pwd)/fill_file"

function random_name(){
	let numofchars=$RANDOM%8+1
	currpos=1
	while [ $numofchars -ge $currpos ]; do
		let i=$RANDOM%$length+1
		currchar=$(expr substr $string $i 1)
		name="$name$currchar"
		let currpos=currpos+1
	done
}

if [ $# -ne 4 ]; then
	echo "Invalid number of arguments"
	exit 1
else
	if [ $2 -lt 0 ]; then
		echo "Invalid number of files"
		exit 1
	elif [ $3 -lt 0 ]; then
		echo "Invalid number of directories"
		exit 1
	elif [ $4 -lt 0 ]; then
		echo "Invalid number of levels"
		exit 1
	fi

	if [ ! -d $1 ]; then
		mkdir "$1"
	fi
fi

declare -a dirnames	#holds directory names to create the files with round robin
cd "$1"
dirnames[0]="$(pwd)"

length=${#string}	#length of string variable
level=0			#current level
dirn=0			#num of created dirs
index=1			#index of array saving directories' names


while [ $dirn -lt $3 ]; do		#created dirs < specified number of dirs
	level=0
	name=""
	while [ $level -lt $4 ]; do
		if [ $dirn -lt $3 ]; then
			random_name		#random directory name
			while [ -e $name ]; do
				name=""
				random_name
			done
		
			mkdir $name
			let dirn=dirn+1
			cd $name
			name=""
			dirnames[$index]="$(pwd)"
			let index=index+1

			let level=level+1
		else
			break
		fi
	done

	while [ $level -gt 0 ]; do
		cd ..
		let level=level-1
	done

done

filescr=0	#files already created
currdir=0
let dirn=dirn+1	#the dirs created plus one for dir_name

while [ $filescr -lt $2 ]; do
	cd "${dirnames[$currdir]}"
	name=""
	random_name
	while [ -e $name ]; do
		name=""
		random_name
	done

	$fillfile $name

	let filescr=filescr+1
	let currdir=currdir+1
	if [ $currdir -gt $dirn ]; then
		currdir=0
	fi

done
