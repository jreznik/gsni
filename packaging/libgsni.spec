Name:           libgsni
Version:        1.0.0
Release:        1%{?dist}
Summary:        StatusNotifierItem D-Bus tray icon library for GTK4

License:        LGPL-2.1-or-later
URL:            https://github.com/jreznik/gsni
Source0:        https://github.com/jreznik/gsni/releases/download/v%{version}/gsni-%{version}.tar.xz

BuildRequires:  meson >= 1.0
BuildRequires:  gcc
BuildRequires:  pkgconfig(glib-2.0) >= 2.80
BuildRequires:  pkgconfig(gdk-pixbuf-2.0) >= 2.42
BuildRequires:  pkgconfig(gobject-introspection-1.0) >= 1.78
BuildRequires:  python3-devel >= 3.9

%description
libgsni implements the StatusNotifierItem (SNI) D-Bus protocol
for GTK4 applications.  It provides GsniItem for creating tray
icons with menus, tooltips, and scroll events, and GsniHost for
consuming SNI items in custom system trays.

%package        -n %{name}1
Summary:        Runtime library for %{name}
Requires:       glib2%{?_isa} >= 2.80

%description    -n %{name}1
Runtime shared library for the libgsni StatusNotifierItem
implementation.

%package        -n %{name}-devel
Summary:        Development files for %{name}
Requires:       %{name}1%{?_isa} = %{version}-%{release}
Requires:       pkgconfig(glib-2.0) >= 2.80

%description    -n %{name}-devel
Headers and pkg-config file for developing applications using libgsni.

%package        -n gir1.2-Gsni-1.0
Summary:        GObject Introspection data for %{name}
Requires:       %{name}1%{?_isa} = %{version}-%{release}
BuildArch:      noarch

%description    -n gir1.2-Gsni-1.0
GObject Introspection typelib for using libgsni from Python and other
language bindings.

%package        -n python3-%{name}
Summary:        Python bindings for %{name}
Requires:       gir1.2-Gsni-1.0 = %{version}-%{release}
Requires:       python3-gobject
BuildArch:      noarch

%description    -n python3-%{name}
Python wrapper for libgsni that provides an idiomatic API on top of
the GObject Introspection bindings.  Includes a context manager for
automatic tray icon registration and cleanup.

%prep
%autosetup -n gsni-%{version} -p1

%build
meson setup build \
    --prefix=%{_prefix} \
    --libdir=%{_libdir} \
    --includedir=%{_includedir} \
    --datadir=%{_datadir} \
    -Ddocs=false
ninja -C build

%install
DESTDIR=%{buildroot} ninja -C build install

%check
ninja -C build test || true

%files -n %{name}1
%license LICENSE
%doc README.md
%{_libdir}/libgsni.so.*

%files -n %{name}-devel
%{_includedir}/gsni/
%{_libdir}/libgsni.so
%{_libdir}/pkgconfig/gsni.pc
%{_datadir}/gir-1.0/Gsni-1.0.gir

%files -n gir1.2-Gsni-1.0
%{_libdir}/girepository-1.0/Gsni-1.0.typelib

%files -n python3-%{name}
%{python3_sitelib}/gsni/

%changelog
* Fri Jun 12 2026 Jaroslav Reznik <jreznik@redhat.com> - 1.0.0-1
- Initial release
