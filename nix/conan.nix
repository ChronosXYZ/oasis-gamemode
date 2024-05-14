{ lib
, stdenv
, python3
, fetchFromGitHub
, git
, pkg-config
, fetchPypi
}:

# Note:
# Conan has specific dependency demands; check
#     https://github.com/conan-io/conan/blob/master/conans/requirements.txt
#     https://github.com/conan-io/conan/blob/master/conans/requirements_server.txt
# on the release branch/commit we're packaging.
#
# Two approaches are used here to deal with that:
# Pinning the specific versions it wants in `newPython`,
# and using `substituteInPlace conans/requirements.txt ...`
# in `postPatch` to allow newer versions when we know
# (e.g. from changelogs) that they are compatible.

let
  newPython = python3.override {
    packageOverrides = self: super: {
      node-semver = super.node-semver.overridePythonAttrs (oldAttrs: rec {
        version = "0.6.1";
        src = fetchPypi {
          pname = "node-semver";
          inherit version;
          hash = "sha256-QBb3wQcbBJPxjbaeoC03Y+mKYzYG18e+yoEeU7WsZrc=";
        };
        doCheck = false;
        pythonImportsCheck = [
          "semver"
        ];
      });
      distro = super.distro.overridePythonAttrs (oldAttrs: rec {
        version = "1.5.0";
        src = oldAttrs.src.override {
          inherit version;
          hash = "sha256-Dlh1auOPvY/DAg1UutuOrhfFudy+04ixe7Vbilko35I=";
        };
      });
    };
  };

in
newPython.pkgs.buildPythonApplication rec {
  pname = "conan";
  version = "1.64.0";

  src = fetchFromGitHub {
    owner = "conan-io";
    repo = "conan";
    rev = "refs/tags/${version}";
    hash = "sha256-roib7iefOLL2MlaNeTYiAgu0vy+HXv8kMmnXAEyKnpk=";
  };

  postPatch = ''
    substituteInPlace conans/requirements.txt \
      --replace 'PyYAML>=3.11, <6.0' 'PyYAML>=3.11'
  '';

  propagatedBuildInputs = with newPython.pkgs; [
    bottle
    colorama
    python-dateutil
    deprecation
    distro
    fasteners
    future
    jinja2
    node-semver
    patch-ng
    pluginbase
    pygments
    pyjwt
    pylint # Not in `requirements.txt` but used in hooks, see https://github.com/conan-io/conan/pull/6152
    pyyaml
    requests
    six
    tqdm
    urllib3
  ] ++ lib.optionals stdenv.isDarwin [
    idna
    cryptography
    pyopenssl
  ];

  nativeCheckInputs = [
    pkg-config
    git
  ] ++ (with newPython.pkgs; [
    codecov
    mock
    nose
    parameterized
    webtest
  ]);

  doCheck = false;
}
