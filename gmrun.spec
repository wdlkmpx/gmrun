Name: gmrun
Version: 0.5.4
Release: 1
Summary: Small 'Run application' X11 utility based on GTK
Group: X11/Utilities
Source: %{name}-%{version}.tar.gz
License: GPL
Packager: Mihai Bazon <mishoo@infoiasi.ro>
BuildPrereq: gtk+ >= 1.2.6

%description

  Short GtkEntry for file autocompletion + main.cc that does the needed stuff
  for running programs.  This is intended as a replacement to grun, which
  (sorry) sucks.  The idea comes from the KDE Window Manager (ALT-F2 in KDE).
  Though, GNOME is better :)

%prep
%setup -q
%build
%configure
./configure
make
%install
make prefix=$RPM_BUILD_ROOT%{prefix} install

%files
%defattr(-,root,root)
%{_bindir}/%{name}
%doc [A-Z][A-Z]* ChangeLog
/usr/share/gmrun/gmrunrc

%changelog
* Thu Jul 19 2001 Mihai Bazon <mishoo@infoiasi.ro>
- added history search capabilities (CTRL-R / CTRL-S, like in bash / Emacs)
- small bug fixes

* Fri Jun 29 2001 Mihai Bazon <mishoo@infoiasi.ro>
- history size configurable from config file
- window appears directly where it should (no more flicker)

* Wed May 14 2001 Mihai Bazon <mishoo@infoiasi.ro>
- added default configuration file (goes to /usr/share/gmrun)

* Wed May 07 2001 Marius Feraru <altblue@n0i.net>
- updated to version 0.5.3 and took over to 0.5.31

* Wed May 03 2001 Marius Feraru <altblue@n0i.net>
- updated to version 0.2.5 and took over to 0.2.51:
	* configuration file with 2 options for now:
		'Terminal' and 'Width'
	* added some more (and hopefully more useful) documentation:
		README.hints, README.gmrunrc and README.icewm

* Wed May 03 2001 Marius Feraru <altblue@n0i.net>
- updated to version 0.2.2:
	* Ctrl-Enter spawns a terminal

* Wed May 02 2001 Marius Feraru <altblue@n0i.net>
- initial RPM build
