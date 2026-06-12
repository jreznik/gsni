Name:           libgsni
Version:        1.0.0
Release:        1%{?dist}
Summary:        StatusNotifierItem D-Bus tray icon library for GTK4

License:        LGPL-2.1-or-later
URL:            https://github.com/jreznik/gsni
Source0:        https://github.com/jreznik/gsni/releases/download/v%{version}/gsni-%{version}.tar.xz

BuildRequires:  meson >= 1.0
BuildRequires:  gcc
BuildRequires:  glib2-devel >= 2.80
BuildRequires:  gdk-pixbuf2-devel >= 2.42
BuildRequires:  gobject-introspection-devel >= 1.78

%description
libgsni implements the StatusNotifierItem (SNI) D-Bus protocol
for GTK4 applications.  It provides GsniItem for creating tray
icons with menus, tooltips, and scroll events, and GsniHost for
consuming SNI items in custom system trays.
Written by Jaroslav Reznik <jreznik@redhat.com>.

%package        -n %{name}1
Summary:        Runtime library for %{name}
Requires:       glib2%{?_isa} >= 2.80

%description    -n %{name}1
Runtime shared library for the libgsni StatusNotifierItem implementation.

%package        -n %{name}-devel
Summary:        Development files for %{name}
Requires:       %{name}1%{?_isa} = %{version}-%{release}
Requires:       glib2-devel%{?_isa} >= 2.80

%description    -n %{name}-devel
Headers and pkg-config file for developing applications using libgsni.

%package        -n gir1.2-gsni-1.0
Summary:        GObject Introspection data for %{name}
Requires:       %{name}1%{?_isa} = %{version}-%{release}

%description    -n gir1.2-gsni-1.0
GObject Introspection typelib for using libgsni from Python and other
language bindings.

%prep
%autosetup -n gsni-%{version}

%build
%meson
%meson_build

%install
%meson_install

%check
%meson_test

%files -n %{name}1
%license LICENSE
%doc README.md
%{_libdir}/libgsni.so.*

%files -n %{name}-devel
%{_includedir}/gsni/
%{_libdir}/libgsni.so
%{_libdir}/pkgconfig/gsni.pc

%files -n gir1.2-gsni-1.0
%{_libdir}/girepository-1.0/Gsni-1.0.typelib

%changelog
* Thu Jun 12 2026 Jaroslav Reznik <jreznik@redhat.com> - 1.0.0-1
- Initial release
