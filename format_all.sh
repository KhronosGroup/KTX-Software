#!/bin/bash

# TODO: Adjust folders and files that needs to be formatted
for file in $(find ./tools ./tests ./utils ./interface ./include ./examples -name '*.h' -or -name '*.hpp' -or -name '*.hh' -or -name '*.c' -or -name '*.cc' -or -name '*.cpp') ./lib/*.{h,c,cc,cpp,hpp,hh};
do
	if [[ -f $file ]]; then
		echo "Formatting file: $file ..."
		clang-format -style=file -i $file
		dos2unix $file
	fi
done
