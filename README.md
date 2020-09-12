GMRUN
-----
A run utiliy featuring a slim design and bash style auto-completion. 

see AUTHORS file

Features
---------

    * Tilda completion (~/ <==> $HOME/)

    * Completion works for separate words (e.g. you can type em<TAB> which
      turns to emacs, type a SPACE, and write the file you want to open using
      completion).

    * Configuration file: ~/.gmrunrc or /etc/gmrunrc.
      Check one of them, configuration is very simple.  From that file you
      can change window position and width, history size, terminal, URL
      handlers, etc.

    * Config file parameter: History.  History is maintained in the file "
      ~/.gmrun_history ".

    * CTRL-Enter runs the command in a terminal.  CTRL-Enter without any
      command starts a new terminal.

    * You can use CTRL-R / CTRL-S to search through history.

    * URL handlers allowing you to enter lines like "http://www.google.com"
      to start your favorite browser on www.google.com.
      The URL-s are configurable from the configuration
      file, in a simple manner (I hope..).

    * Extension handlers (added in 0.8.0).  Basically you can run, for
      instance, a ".txt" file, assuming that you have configured a handler for
      it in the configuration file.  The default handler for ".txt" files is,
      of course, Emacs.  But you can easily change that, you... you VIM user!

Requirements
-------------

    * A C++ compiler
    * GTK-2 / 3

Compilation, installation
--------------------------

    Use the configure script (run ./autogen.sh if ./configure is missing):

        $ ./configure --prefix=/usr
        $ make
        $ make install

    Pass --enable-gtk3 to ./configure to build the gtk3 ui

    Optionally you can configure your window manager to call gmrun
    with WinKey + R or something.

Tips and tricks
---------------

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

