Copyright (c) 2000-2003 Mihai Bazon <mishoo@infoiasi.ro>

This program falls under the GNU General Public License version 2 or above.

Features
---------

    * Tilda completion (~/ <==> $HOME/)

    * Completion works for separate words (e.g. you can type em<TAB> which
      turns to emacs, type a SPACE, and write the file you want to open using
      completion).

    * I added history capability (limited to 20 entries, change history.cc for
      more, #define HIST_MAX_SIZE).  History is maintained in the file "
      ~/.gmrun_history ".

      CHANGED (since 0.5.3, I think..) -- new config file parameter: History.
    
    * CTRL-Enter runs the command in a terminal.  CTRL-Enter without any
      command starts a new terminal.

    * New configuration file: ~/.gmrunrc or /usr/local/share/gmrun/gmrunrc.
      Check one of them, configuration is very simple.  From that file you
      can change window position and width, history size, terminal, URL
      handlers, etc.

    * You can use CTRL-R / CTRL-S to search through history, much like in bash
      or Emacs.  Also, pressing "!" as the first character enters some special
      search mode, finding those history entries that begin with the entered
      text.

    * URL handlers (added in 0.6.0).  Nice feature, allowing you to enter
      lines like "http://www.google.com" to start your favorite browser on
      www.google.com.  The URL-s are configurable from the configuration
      file, in a simple manner (I hope..).

    * Extension handlers (added in 0.8.0).  Basically you can run, for
      instance, a ".txt" file, assuming that you have configured a handler for
      it in the configuration file.  The default handler for ".txt" files is,
      of course, Emacs.  But you can easily change that, you... you VIM user!

    * UTF-8 support (added in 0.9.2)

Requirements
-------------

    * A C++ compiler
    * GTK-2

Compilation, installation
--------------------------

    Use the configure script (run ./autogen.sh if ./configure is missing):

        $ ./configure
        $ make
        $ make install

    After this the executable goes usually in /usr/local/bin, make sure this is
    in your path.  Put this in your .sawmillrc:

        (require 'sawmill-defaults)
        (bind-keys global-keymap "S-C-M-RET" '(system "gmrun &"))

    Note that if you're using sawfish you have other ways to customize your
    keyboard, through the control panel.

    For another window managers you gonna have to find a way to bind this
    program to a key combination; otherwise it wouldn't be much help... (I
    coded it exactly to get rid of the mouse-time-wasting-walking).
    E.g. for IceWM (my favorite, at this time) you edit ~/.icewm/preferences
    and add a line like:

        KeySysRun="Alt+Ctrl+Shift+Enter"


Tips and tricks (hope that doesn't sound MS-ish...)
----------------------------------------------------

1. Everything that doesn't start with "/" or "~" gets completed from $PATH
   environment var.  More exactly, all files from $PATH directories are
   subjects to completion (even if they are NOT executables; this is a
   bug, but I'm afraid I'm not willing to fix it).

   Pressing TAB once when no text is entered opens the completion window,
   which will contain ALL files under $PATH.

2. For instance you use TAB to complete from "nets" to "netscape-navigator".
   A small window appears, allowing you to select from:

       - netscape
       - netscape-communicator
       - netscape-navigator
       - netstat

   That is because all these executables have the same prefix, "nets".  You
   type TAB twice to get to the third element ("netscape-navigator").  Now,
   if you want to add a parameter such as "-url http://blah.blah.org" you
   can press SPACE (the list disappears, and a SPACE is inserted after the
   netscape-navigator).

3. If you accidentally pressed TAB more than you wanted (in that small
   window, described above) you can use UP / DOWN arrows to select the right
   completion.

4. - ESC closes the completion window, leaving the selected text in the entry.
   - HOME / END - the same, but clears the selection.
   - SPACE - the same, but clears the selection and appends one space.
   - Pressing ENTER (anytime) runs the command that is written in the entry.
   - Pressing CTRL+Enter (anytime) runs the command in a terminal (check your
     configuration file).  But if the entry is empty (no text is present, or
     only whitespaces) then a fresh terminal will be started.

5. Suppose you use CTRL-R to search backwards through history.  If,
   accidentally, you skipped the line that you're interested in, you can use
   CTRL-S to search forward.  This is more awesome than in bash :)  It
   basically acts like a filter on history, for which you use CTRL-R instead
   of UP arrow, and CTRL-S instead of DOWN arrow.

   The same if you search something with "!": only lines that BEGIN with the
   entered text are matching, but you can reverse the search order using
   CTRL-R / CTRL-S.  Very flexible approach.

