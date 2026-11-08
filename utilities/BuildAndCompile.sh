#!/usr/bin/env bash
set -Eeuo pipefail
trap 'status=$?; printf "[%s] ERROR at line %s: %s (exit %s)\n" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$LINENO" "$BASH_COMMAND" "$status" >&2' ERR

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd)"
cd -- "$PROJECT_ROOT"

run_as_root() {
  if [[ "$EUID" -eq 0 ]]; then
    "$@"
  elif command -v sudo >/dev/null 2>&1; then
    sudo "$@"
  else
    echo "Install dependencies as root or install sudo first." >&2
    return 1
  fi
}

install_linux_dependencies() {
  local os_release=/etc/os-release
  local distro_id=""
  local distro_like=""

  if [[ ! -r "$os_release" ]]; then
    echo "Cannot detect the Linux distribution: $os_release is missing." >&2
    return 1
  fi

  # shellcheck disable=SC1091
  source "$os_release"
  distro_id="${ID:-unknown}"
  distro_id="${distro_id,,}"
  distro_like="${ID_LIKE:-}"
  distro_like="${distro_like,,}"

  echo "[$(date -u '+%Y-%m-%dT%H:%M:%SZ')] Installing dependencies for ${PRETTY_NAME:-$distro_id}"
  if [[ "$distro_id" == "ubuntu" || "$distro_id" == "debian" || "$distro_like" == *debian* ]]; then
    run_as_root apt-get update
    run_as_root apt-get install -y \
      build-essential cmake gcc g++ pkg-config dpkg-dev rpm \
      libasound2-dev libjack-jackd2-dev ladspa-sdk-dev \
      freeglut3-dev libfreetype6-dev libx11-dev libxext-dev \
      libxinerama-dev libxrandr-dev libxcursor-dev \
      libcurl4-openssl-dev libxcb-util-dev libxcb-cursor-dev \
      libxcb-keysyms1-dev libxcb-xinerama0-dev libxcb-xkb-dev \
      libxcb-render0-dev libxcb-shape0-dev libxkbcommon-dev \
      libxkbcommon-x11-dev libgtkmm-3.0-dev
  elif [[ "$distro_id" == "fedora" || "$distro_like" == *fedora* || "$distro_like" == *rhel* ]]; then
    local package_manager
    if command -v dnf >/dev/null 2>&1; then
      package_manager=dnf
    elif command -v yum >/dev/null 2>&1; then
      package_manager=yum
    else
      echo "Fedora/RHEL-like distribution detected, but neither dnf nor yum is available." >&2
      return 1
    fi
    run_as_root "$package_manager" install -y \
      gcc gcc-c++ make cmake pkgconf-pkg-config dpkg dpkg-devel rpm-build \
      alsa-lib-devel jack-audio-connection-kit-devel ladspa-devel \
      freeglut-devel freetype-devel libX11-devel libXext-devel \
      libXinerama-devel libXrandr-devel libXcursor-devel libcurl-devel \
      libxcb-devel xcb-util-devel xcb-util-cursor-devel \
      xcb-util-keysyms-devel xcb-util-wm-devel libxkbcommon-devel gtkmm30-devel
  elif [[ "$distro_id" == "opensuse" || "$distro_id" == "opensuse-leap" || \
          "$distro_id" == "opensuse-tumbleweed" || "$distro_like" == *suse* ]]; then
    run_as_root zypper --non-interactive refresh
    run_as_root zypper --non-interactive install --no-recommends \
      gcc gcc-c++ make cmake pkg-config dpkg dpkg-devel rpm-build \
      alsa-devel libjack-devel ladspa-devel freeglut-devel freetype2-devel \
      libX11-devel libXext-devel libXinerama-devel libXrandr-devel \
      libXcursor-devel libcurl-devel libxcb-devel xcb-util-devel \
      xcb-util-cursor-devel xcb-util-keysyms-devel xcb-util-wm-devel \
      libxkbcommon-devel gtkmm3-devel
  elif [[ "$distro_id" == "arch" || "$distro_like" == *arch* ]]; then
    run_as_root pacman -Sy --needed --noconfirm \
      base-devel cmake pkgconf dpkg rpm-tools \
      alsa-lib jack2 ladspa freeglut freetype2 libx11 libxext \
      libxinerama libxrandr libxcursor curl libxcb xcb-util \
      xcb-util-cursor xcb-util-keysyms xcb-util-wm libxkbcommon gtkmm3
  else
    echo "Unsupported Linux distribution: ${PRETTY_NAME:-$distro_id}." >&2
    echo "Supported package managers: apt (Debian/Ubuntu), dnf/yum (Fedora/RHEL), zypper (openSUSE), pacman (Arch)." >&2
    return 1
  fi
}

install_linux_dependencies
echo "[$(date -u '+%Y-%m-%dT%H:%M:%SZ')] Starting: Fetch Latest External SDKs"
trap 'status=$?; printf "[%s] Finished: Fetch Latest External SDKs (exit %s)\n" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$status"' EXIT
mkdir -p externals
rm -rf -- externals/JUCE externals/VST3_SDK externals/xsimd

# Nettoyage des en-t�tes d'authentification pour �viter les conflits public/priv�
git config --global --unset-all http.https://github.com || true

# Reconstruction dynamique des URL
BASE_URL="https://github.com"
JUCE_REPO="${BASE_URL}/juce-framework/JUCE.git"
VST3_REPO="${BASE_URL}/steinbergmedia/vst3sdk.git"
XSIMD_REPO="${BASE_URL}/xtensor-stack/xsimd.git"

echo "Clonage de JUCE..."
git clone --depth 1 "$JUCE_REPO" externals/JUCE

echo "Clonage du SDK VST3..."
git clone --depth 1 "$VST3_REPO" externals/VST3_SDK

echo "Initialisation des sous-modules VST3..."
cd externals/VST3_SDK
git submodule update --init --recursive --depth 1
cd ../..

echo "Clonage de xsimd..."
git clone --depth 1 "$XSIMD_REPO" externals/xsimd

echo "[$(date -u '+%Y-%m-%dT%H:%M:%SZ')] Starting: Configure CMake"
trap 'status=$?; printf "[%s] Finished: Configure CMake (exit %s)\n" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$status"' EXIT
cmake -S . -B build \
-DCMAKE_BUILD_TYPE=Release \
-DSMG_BUILD_EXAMPLES=OFF \
-DJUCE_BUILD_EXAMPLES=OFF \
-DJUCE_BUILD_EXTRAS=OFF \
-DHELMBOY_RUN_TESTS="${HELMBOY_RUN_TESTS:-OFF}"

echo "[$(date -u '+%Y-%m-%dT%H:%M:%SZ')] Starting: Build and package VST3"
trap 'status=$?; printf "[%s] Finished: Build and package VST3 (exit %s)\n" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$status"' EXIT
cmake --build build --config Release --target HelmBoyPlugin_VST3 --parallel $(nproc)
VST3_BUNDLE="$(find build -type d -name "helmBoy.vst3" -print -quit)"
test -n "$VST3_BUNDLE"
test -f "$VST3_BUNDLE/Contents/x86_64-linux/helmBoy.so"
rm -rf dist/VST3
mkdir -p dist/VST3
cp -r "$VST3_BUNDLE" dist/VST3/
tar -czf helmboy-linux-vst3.tar.gz -C dist/VST3 helmBoy.vst3

echo "[$(date -u '+%Y-%m-%dT%H:%M:%SZ')] Starting: Build and package LV2"
trap 'status=$?; printf "[%s] Finished: Build and package LV2 (exit %s)\n" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$status"' EXIT
cmake --build build --config Release --target HelmBoyPlugin_LV2 --parallel $(nproc)
LV2_BUNDLE="$(find build -type d -name "helmBoy.lv2" -print -quit)"
test -n "$LV2_BUNDLE"
test -f "$LV2_BUNDLE/libhelmBoy.so"
rm -rf dist/LV2
mkdir -p dist/LV2
cp -r "$LV2_BUNDLE" dist/LV2/
tar -czf helmboy-linux-lv2.tar.gz -C dist/LV2 helmBoy.lv2

echo "[$(date -u '+%Y-%m-%dT%H:%M:%SZ')] Starting: Build and package Standalone"
trap 'status=$?; printf "[%s] Finished: Build and package Standalone (exit %s)\n" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$status"' EXIT
cmake --build build --config Release --target HelmBoyStandalone --parallel $(nproc)
mkdir -p dist/Standalone
find build -type f -executable -name "helmBoy" -exec cp {} dist/Standalone/ \;
test -n "$(find dist/Standalone -mindepth 1 -maxdepth 1 -print -quit)"
tar -czf helmboy-linux-standalone.tar.gz -C dist/Standalone .

echo "[$(date -u '+%Y-%m-%dT%H:%M:%SZ')] Starting: Package global archive"
trap 'status=$?; printf "[%s] Finished: Package global archive (exit %s)\n" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$status"' EXIT
tar -czvf helmboy-linux-artifacts.tar.gz -C dist .

echo "[$(date -u '+%Y-%m-%dT%H:%M:%SZ')] Starting: Build standalone, LV2 and VST3 DEB/RPM packages"
trap 'status=$?; printf "[%s] Finished: Build standalone, LV2 and VST3 DEB/RPM packages (exit %s)\n" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$status"' EXIT

PACKAGE_VERSION="$(tr -d '\r' < CMakeLists.txt | sed -n 's/^[[:space:]]*VERSION[[:space:]]\([0-9][0-9.]*\)$/\1/p' | head -n 1)"
test -n "$PACKAGE_VERSION"
PACKAGE_TEMP="${RUNNER_TEMP:-${TMPDIR:-/tmp}}"
DEB_ARCH="$(dpkg --print-architecture)"
mkdir -p dist/packages

create_linux_package_pair() {
 local package_name="$1"
 local package_root="$2"
 local elf_path="$3"
 local package_files="$4"
 local description="$5"
 local requires_presets="$6"
 local is_arch_independent="$7"
 local deb_root="$PACKAGE_TEMP/${package_name}-deb-root"
 local deb_metadata="$PACKAGE_TEMP/${package_name}-debian"
 local deb_dependencies=""
 local rpm_topdir="$PACKAGE_TEMP/${package_name}-rpmbuild"
 local rpm_requires=""
 local rpm_build_arch=""
 local package_license="/usr/share/doc/${package_name}/copyright"
 local deb_arch="$DEB_ARCH"

 if [[ "$is_arch_independent" == "yes" ]]; then
   deb_arch=all
   rpm_build_arch="BuildArch: noarch"
 fi
 mkdir -p "$deb_root/DEBIAN" "$deb_metadata/debian"
 cp -a "$package_root/." "$deb_root/"
 if [[ -n "$elf_path" ]]; then
   cat > "$deb_metadata/debian/control" <<EOF
Source: $package_name
Section: sound
Priority: optional
Maintainer: HelmBoy Contributors
Standards-Version: 4.6.2

Package: $package_name
Architecture: $deb_arch
Description: $description
 Factory presets and supporting files for HelmBoy.
EOF
   deb_dependencies="$(cd "$deb_metadata" && dpkg-shlibdeps -O -p"$package_name" -e"$deb_root$elf_path" | sed -n 's/^shlibs:Depends=//p')"
   test -n "$deb_dependencies"
 fi
 if [[ "$requires_presets" == "yes" ]]; then
   deb_dependencies="helmboy-presets (= $PACKAGE_VERSION), $deb_dependencies"
 fi

 cat > "$deb_root/DEBIAN/control" <<EOF
Package: $package_name
Version: $PACKAGE_VERSION
Section: sound
Priority: optional
Architecture: $deb_arch
Maintainer: HelmBoy Contributors
Description: $description
 HelmBoy $description.
Homepage: https://github.com/Hans45/helmBoy
EOF
 if [[ -n "$deb_dependencies" ]]; then
   printf 'Depends: %s\n' "$deb_dependencies" >> "$deb_root/DEBIAN/control"
 fi
 dpkg-deb --build --root-owner-group "$deb_root" \
   "dist/packages/${package_name}_${PACKAGE_VERSION}_${deb_arch}.deb"

 mkdir -p "$rpm_topdir"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
 tar -czf "$rpm_topdir/SOURCES/${package_name}-payload.tar.gz" -C "$package_root" .
 if [[ "$requires_presets" == "yes" ]]; then
   rpm_requires="Requires: helmboy-presets = $PACKAGE_VERSION-1"
 fi
 cat > "$rpm_topdir/SPECS/${package_name}.spec" <<EOF
Name: $package_name
Version: $PACKAGE_VERSION
Release: 1
Summary: $description
License: GPL-3.0-or-later
$rpm_build_arch
$rpm_requires
Source0: ${package_name}-payload.tar.gz

%description
HelmBoy $description.

%prep

%build

%install
mkdir -p %{buildroot}
tar -xzf %{SOURCE0} -C %{buildroot}

%files
%defattr(-,root,root,-)
%license $package_license
$package_files
EOF
 rpmbuild --define "_topdir $rpm_topdir" -bb "$rpm_topdir/SPECS/${package_name}.spec"
 find "$rpm_topdir/RPMS" -type f -name "${package_name}-*.rpm" \
   -exec cp {} dist/packages/ \;
 test -n "$(find dist/packages -maxdepth 1 -type f -name "${package_name}-*.rpm" -print -quit)"
}

PRESETS_ROOT="$PACKAGE_TEMP/helmboy-presets-root"
mkdir -p "$PRESETS_ROOT/usr/share/helmBoy/patches" \
 "$PRESETS_ROOT/usr/share/doc/helmboy-presets"
cp -a patches/. "$PRESETS_ROOT/usr/share/helmBoy/patches/"
find "$PRESETS_ROOT/usr/share/helmBoy/patches" -name .DS_Store -delete
rm -f "$PRESETS_ROOT/usr/share/helmBoy/patches/.gitignore"
install -m 0644 COPYING "$PRESETS_ROOT/usr/share/doc/helmboy-presets/copyright"
create_linux_package_pair \
 helmboy-presets "$PRESETS_ROOT" "" \
 "/usr/share/helmBoy/patches" \
 "HelmBoy factory presets" no yes

STANDALONE_ROOT="$PACKAGE_TEMP/helmboy-standalone-root"
mkdir -p "$STANDALONE_ROOT/usr/bin" \
 "$STANDALONE_ROOT/usr/share/applications" \
 "$STANDALONE_ROOT/usr/share/icons/hicolor/128x128/apps" \
 "$STANDALONE_ROOT/usr/share/doc/helmboy-standalone"
install -m 0755 dist/Standalone/helmBoy "$STANDALONE_ROOT/usr/bin/helmBoy"
install -m 0644 images/helmBoy_icon_128_1x.png \
 "$STANDALONE_ROOT/usr/share/icons/hicolor/128x128/apps/helmBoy.png"
install -m 0644 COPYING "$STANDALONE_ROOT/usr/share/doc/helmboy-standalone/copyright"
cat > "$STANDALONE_ROOT/usr/share/applications/helmboy.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=HelmBoy
Comment=Polyphonic software synthesizer
Exec=helmBoy
TryExec=helmBoy
Icon=helmBoy
Categories=AudioVideo;Audio;Midi;
Terminal=false
EOF
create_linux_package_pair \
 helmboy-standalone "$STANDALONE_ROOT" /usr/bin/helmBoy \
 $'/usr/bin/helmBoy\n/usr/share/applications/helmboy.desktop\n/usr/share/icons/hicolor/128x128/apps/helmBoy.png' \
 "standalone synthesizer" yes no

VST3_BUNDLE="$(find dist/VST3 -mindepth 1 -maxdepth 1 -type d -name 'helmBoy.vst3' -print -quit)"
test -n "$VST3_BUNDLE"
VST3_ELF="$(find "$VST3_BUNDLE" -type f -name 'helmBoy.so' -print -quit)"
test -n "$VST3_ELF"
VST3_ELF_RELATIVE="${VST3_ELF#"$VST3_BUNDLE"/}"
VST3_ROOT="$PACKAGE_TEMP/helmboy-vst3-root"
mkdir -p "$VST3_ROOT/usr/lib/vst3" "$VST3_ROOT/usr/share/doc/helmboy-vst3"
cp -a "$VST3_BUNDLE" "$VST3_ROOT/usr/lib/vst3/"
install -m 0644 COPYING "$VST3_ROOT/usr/share/doc/helmboy-vst3/copyright"
create_linux_package_pair \
 helmboy-vst3 "$VST3_ROOT" "/usr/lib/vst3/helmBoy.vst3/$VST3_ELF_RELATIVE" \
 "/usr/lib/vst3/helmBoy.vst3" \
 "VST3 synthesizer plugin" yes no

LV2_BUNDLE="$(find dist/LV2 -mindepth 1 -maxdepth 1 -type d -name 'helmBoy.lv2' -print -quit)"
test -n "$LV2_BUNDLE"
LV2_ELF="$LV2_BUNDLE/libhelmBoy.so"
test -f "$LV2_ELF"
LV2_ROOT="$PACKAGE_TEMP/helmboy-lv2-root"
mkdir -p "$LV2_ROOT/usr/lib/lv2" "$LV2_ROOT/usr/share/doc/helmboy-lv2"
cp -a "$LV2_BUNDLE" "$LV2_ROOT/usr/lib/lv2/"
install -m 0644 COPYING "$LV2_ROOT/usr/share/doc/helmboy-lv2/copyright"
create_linux_package_pair \
 helmboy-lv2 "$LV2_ROOT" /usr/lib/lv2/helmBoy.lv2/libhelmBoy.so \
 "/usr/lib/lv2/helmBoy.lv2" \
 "LV2 synthesizer plugin" yes no
