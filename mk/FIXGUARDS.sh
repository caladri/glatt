#! /bin/sh

fixguards() {
	files=`find . -name '*.h' | cut -d / -f 2- | grep -v core/queue.h | xargs`

	for _file in ${files}; do
		_guard=_`echo ${_file} | tr a-z/. A-Z__`_
		( echo '#ifndef	'${_guard} ; echo '#define	'${_guard} ; ( cat ${_file} | tail -n +3 | sed '$d' ) ; echo '#endif /* !'${_guard}' */' ) > x
		cat x > ${_file}
		rm x
	done
}

# Platforms and CPUs have their own guard hierarchies.
fixguards
for n in platform/* cpu/*; do
	if [ -d $n ]; then
		(cd $n && fixguards)
	fi
done
