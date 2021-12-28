#!/bin/bash
if [ -z "${3}" ]
then
  echo "引数が足りない" >&2
  exit 1
fi

PYENV_VERSION=${1}
PYENV_ROOT=${2}
BUILD_DIR=${3}

function die(){
  echo $@ >&2
  exit 1
}

if [ "${PYENV_VERSION}" != "native" ]
then
  BUILD_SUFFIX=${PYENV_VERSION}-$(echo ${PYENV_ROOT}|sha256sum|sed -e 's/\s\+.*//')
  if [ ! -d "${PYENV_ROOT}" ]
  then
    git clone --depth 1 https://github.com/pyenv/pyenv.git ${PYENV_ROOT} || die "pyenvをcloneできない"
  fi
  export PYENV_ROOT="${PYENV_ROOT}"
  export PATH="${PYENV_ROOT}/bin:${PATH}"
  eval "$(pyenv init -)"
  CFLAGS="-fPIC" PYTHON_CONFIGURE_OPTS="--enable-shared" pyenv install -s ${PYENV_VERSION} || die "Python-${PYENV_VERSION}をインストールできない"
  pyenv local ${PYENV_VERSION} || die "Python-${1}に切り替えられない"
  mkdir -p ${BUILD_DIR}-${BUILD_SUFFIX}
  pushd ${BUILD_DIR}-${BUILD_SUFFIX}
  cmake ../ ${@:4} || die "cmakeできない"
  make -j8 || die "makeできない"
  make test || die "testが失敗した"
  popd
else
  pushd ${BUILD_DIR}
  cmake ../ ${@:4} || die "cmakeできない"
  make -j8 || die "makeできない"
  make test || die "testが失敗した"
  make wyvern_doc || die "ドキュメントを作れない"
  make package || die "パッケージを作れない"
  popd
fi
