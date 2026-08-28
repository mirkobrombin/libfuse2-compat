Name:           libfuse2-compat
Version:        0.1.1
Release:        1%{?dist}
Summary:        FUSE2 ABI bridge for legacy AppImages on FUSE3
License:        LGPL-2.1-or-later
URL:            https://github.com/mirkobrombin/libfuse2-compat
Source0:        %{name}-%{version}.tar.gz
BuildRequires:  gcc
BuildRequires:  make
Requires:       fuse3-libs
Conflicts:      fuse-libs

%description
A libfuse.so.2 compatibility implementation translating the legacy
AppImageKit low-level ABI to libfuse3.

%prep
%autosetup

%build
%make_build

%check
%make_build check

%install
%make_install PREFIX=%{_prefix} LIBDIR=%{_libdir}

%files
%license LICENSE
%doc README.md docs/
%{_libdir}/libfuse.so.2
%{_libdir}/libfuse.so.2.9.9

%changelog
* Fri Aug 28 2026 Mirko Brombin <mirko@fabricators.ltd> - 0.1.1-1
- Build amd64 and arm64 Debian packages

* Thu Aug 27 2026 Mirko Brombin <mirko@fabricators.ltd> - 0.1.0-1
- Initial package
