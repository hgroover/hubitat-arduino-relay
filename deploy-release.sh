#!/bin/sh
# Deploy to release dir
QTBINDIR=/c/Qt/Qt-5.11.1/bin
PROJNAME=RelayControlTest
BUILDTYPE=Desktop_Qt_5_11_1_MSVC2017_64bit
RELTYPE=Release
RELTYPELC=release
BUILDDIR=build-${PROJNAME}-${BUILDTYPE}-${RELTYPE}/${RELTYPELC}
EXEPATH=${PROJNAME}.exe
TMPDIR=build-deployment-${RELTYPELC}

[ -r ${BUILDDIR}/${EXEPATH} ] || { echo "Did not find ${EXEPATH} in ${BUILDDIR}"; exit 1; }

OPT=$1
if [ "${OPT}" = "lite" ]
then
	echo "lite specified, only copying exe"
	LITE=1
else
	LITE=0
fi

if [ ${LITE} = 0 ]
then
  rm -rf ${TMPDIR}
fi
[ -d ${TMPDIR} ] || mkdir -p ${TMPDIR}
cp -p ${BUILDDIR}/${EXEPATH} ${TMPDIR}
if [ ${LITE} = 1 ]
then
	ls -rtl ${TMPDIR}/ | tail -6
else
${QTBINDIR}/windeployqt.exe --verbose 1 --${RELTYPELC} ${TMPDIR}/${EXEPATH}
# Copy any additional data dirs
#cp -p -r ${PROJNAME}/audio ${TMPDIR}/
ls -l ${TMPDIR}
fi

