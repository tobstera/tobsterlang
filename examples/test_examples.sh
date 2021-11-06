#!/bin/bash

for src_file in *.xml; do
	src_filename=$(basename "${src_file}" .xml)
	obj_file="${src_filename}.o"

	tobsterlang "${src_file}" -o "${obj_file}" -O3 >/dev/null
	clang-11 -o "${src_filename}" "${obj_file}" >/dev/null

	in_file="${src_filename}.input"
	out_file="${src_filename}.output"

	diff "${out_file}" <(./"${src_filename}" < "${in_file}") || { echo "${src_filename} produced incorrect output"; exit 1; }
done

