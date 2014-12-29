
for filename in $@; do
	extension=${filename##*.}

	case $extension in
		gif )	echo "$filename is a gif file";;
		txt )  echo "$filename is a text file";;
		xpm )  echo "$filename is a xpm file";;
		jpeg ) echo "$filename is a jpeg file";;
		* )  echo "$filename is normal  file";;
	esac
done
