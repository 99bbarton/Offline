#
# $Id: setup.sh,v 1.67 2013/10/22 23:35:39 brownd Exp $
# $Author: brownd $
# $Date: 2013/10/22 23:35:39 $
#
# Original author Rob Kutschke
#
# Setup the environment to build or use a full release of the Mu2e software.
# This checks that you have already established the Mu2e environment.
#

if [ "`basename $0 2>/dev/null`" = "setup.sh" ];then
    echo "You should be sourcing this file, not executing it."
    exit 1
fi

if [ "${MU2E}" = '' ];then
    echo "The environment variable MU2E is not set."
    echo "You must setup the local Mu2e environment before sourcing this script."
    return 1
fi

# Protect against multiple invocation.
if [ "${MU2E_BASE_RELEASE}" != '' ];then
    echo "A base release has already been setup.  Hope that's OK."
    echo "The base release is: " ${MU2E_BASE_RELEASE}
    return 1
fi

# A very ill-defined state.  We have  a test release but no base release!
if [ "${MU2E_TEST_RELEASE}" != '' ];then
    echo "ERROR: A test release has already been setup but there is no base release."
    echo "Suggest that you log out, log in and restart from the beginning."
    echo "The test release is: " ${MU2E_TEST_RELEASE}
    return 1
fi

# Define the directory in which this file lives as the root of a release.
export MU2E_BASE_RELEASE=`cd "$(dirname ${BASH_SOURCE})" >/dev/null 2>&1 && /bin/pwd`
echo "Base release directory is: " $MU2E_BASE_RELEASE

# Remove any test release environment.  TODO: test this and abort.
export MU2E_SEARCH_PATH=$MU2E_BASE_RELEASE/:$MU2E_DATA_PATH/
echo "MU2E_SEARCH_PATH:   "  $MU2E_SEARCH_PATH

# Setup the framework and its dependent products
setup -B art v1_08_09 -q+e4:+mu2e:+prof

# Geant4 and its cross-section files.
setup -B geant4 v4_9_6_p02 -q+e4:+prof

# Other libraries we need.
setup -B heppdt v3_04_01 -q+e4:+prof
setup -B splines v1_02_01 -q+e4:+prof

# The build system.
setup -B scons v2_3_0a


# Search path for fcl files
export FHICL_FILE_PATH=${MU2E_BASE_RELEASE}:${MU2E_BASE_RELEASE}/fcl

# Tell the framework to look in the local area to find modules.
source ${MU2E_BASE_RELEASE}/bin/setup_mu2e_project.sh

# Check out the BaBar code.
# First build the symlink directory.  Then checkout the code.
babarversion=605
if [  -f "./BaBar/makeInclude.sh" ]; then
  source ./BaBar/makeInclude.sh
  if [ ! -f "BaBar/BaBar/include/BaBar.hh" ]; then
   echo "Checking out the BaBar Kalman Filter code."
   ./BaBar/checkout.sh "-r ${babarversion}"
  else
    # Skip check during grid jobs or else they will DOS attack the svn repository.
    if [ -z "${PROCESS}" ] || [ "${PROCESS}" == 0 ]; then
      ./BaBar/checkVersion.sh ${babarversion}
    else
      echo "Grid job with PROCESS != 0 detected. Skipping version check of BaBar code."
    fi
  fi
fi

#
if [ "${MU2E_BASE_RELEASE}" != `pwd` ]; then
  source ${MU2E_BASE_RELEASE}/bin/addlocal.sh
fi

# A hack that we hope can go away soon.
export G4LIBDIR=$G4LIB/$G4SYSTEM
