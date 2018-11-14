#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/DBoperate.o \
	${OBJECTDIR}/dbrecord.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-Wl,-rpath,../ODBCManage/../bin -L../ODBCManage/../bin -lODBCManage -Wl,-rpath,../AES/../bin -L../AES/../bin -lAES

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ../bin/libDBRecord.${CND_DLIB_EXT}

../bin/libDBRecord.${CND_DLIB_EXT}: ../ODBCManage/../bin/libODBCManage.so

../bin/libDBRecord.${CND_DLIB_EXT}: ../AES/../bin/libAES.so

../bin/libDBRecord.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ../bin
	${LINK.c} -o ../bin/libDBRecord.${CND_DLIB_EXT} ${OBJECTFILES} ${LDLIBSOPTIONS} -shared -fPIC

${OBJECTDIR}/DBoperate.o: DBoperate.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -g -I/usr/include/mysql -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/DBoperate.o DBoperate.c

${OBJECTDIR}/dbrecord.o: dbrecord.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -g -I/usr/include/mysql -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/dbrecord.o dbrecord.c

# Subprojects
.build-subprojects:
	cd ../ODBCManage && ${MAKE}  -f Makefile CONF=Debug
	cd ../AES && ${MAKE}  -f Makefile CONF=Debug

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ../bin/libDBRecord.${CND_DLIB_EXT}

# Subprojects
.clean-subprojects:
	cd ../ODBCManage && ${MAKE}  -f Makefile CONF=Debug clean
	cd ../AES && ${MAKE}  -f Makefile CONF=Debug clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
