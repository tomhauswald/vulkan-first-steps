#!/bin/bash
rm -rf spirv
mkdir -p spirv
for ext in vert frag; do
	files=*.${ext}
	numfiles=$(echo ${files} | wc -w)
	echo "Compiling ${numfiles} ${ext} shaders..."
	i=1
	for file in ${files}; do
		outfile=./spirv/${ext}-${file%%.*}.spv 
		echo "    ${i}/${numfiles}: ${file} => ${outfile}"
		glslc ${file} -o ${outfile}
		i=$((i+1))
	done
	echo ""
done
