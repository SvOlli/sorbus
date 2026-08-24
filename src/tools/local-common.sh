
if [ -n "${RUNNER_NAME:-}" ]; then
   # workaround for github actions: allow stow to fail, hoping enough was done.
   on_stow_fail="true"
   # also preset the intended answer here
   answer="yes"
else
   on_stow_fail="false"
fi

set -eu
cd "$(dirname "${0}")/../.."

readonly BUILD_DIR="$(dirname "${PWD}")/local"
readonly LOCAL_DIR="/usr/local"
readonly STOW_DIR="${LOCAL_DIR}/stow"
JOBS="$(nproc || echo 4)"

if [ $(id -u) -eq 0 ]; then
   echo "Please don't run this as root."
   echo "Run it as a user that's allowed to use 'sudo'."
   exit 12
fi

if [ -w "${STOW_DIR}/${PACKAGE}" -o -w "${STOW_DIR}" ]; then
   message_package="directory is writable, so no sudo required for installation"
   sudo_package=""
else
   message_package="installation requires sudo"
   sudo_package="sudop"
fi

if [ -w "${LOCAL_DIR}/bin" ]; then # check is not 100%
   message_links="creating links is possible, so no sudo required"
   sudo_links=""
else
   message_links="need sudo to create links"
   sudo_links="sudop"
fi

cat <<EOM
This script downloads, compiles (when required) and installs
${PACKAGE}

Download and compilation will be done in
${BUILD_DIR}

Installation will be done into
${STOW_DIR}/${PACKAGE}
${message_package}

Links will be created in
${LOCAL_DIR}
${message_links}

EOM

if [ -z "${answer:-}" ]; then
   read -p "Continue? " answer
fi
case "${answer}" in
y*|Y*) ;;
*) exit 0;;
esac

# wrapper printing what to be done with sudo
sudop()
{
   echo sudo "${@}"
   sudo "${@}"
}

stow_package()
{
   cd "${STOW_DIR}"
   ${sudo_links} stow -D "${PACKAGE_BASE}"-* || ${on_stow_fail}
   ${sudo_links} stow -v "${PACKAGE}"        || ${on_stow_fail}
}

# now, let's prepare start of build
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"
