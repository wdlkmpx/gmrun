gmrun 0.9.2                  http://students.infoiasi.ro/~mishoo/site/gmrun.epl
------------                ---------------------------------------------------

            Short GtkEntry for file autocompletion + main.cc that does the
            needed stuff for running programs.  This is intended as a
            replacement to grun or gnome-run, which (sorry) sucks.  The idea
            comes from the KDE Window Manager (ALT-F2 in KDE).  Though, GNOME
            is better :)


Copyright (c) 2000-2003 Mihai Bazon
Author: Mihai Bazon <mishoo@infoiasi.ro>.

send postcards to:

     Mihai Bazon,
     str. Republicii, nr. 350, sc. E, ap. 9, cod 6500 VASLUI - VASLUI
     Romania

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

    * A good C++ compiler (that is, g++ 3.0 or later).  It did originally
    compile with g++ 2.95, but not anymore ;-] -- though that's easy to fix.

    * GTK-2.  gmrun upto and including 0.8.1 were for GTK-1.x series, version
    0.9.0 requires GTK-2.


For code critics
-----------------

    This program is written in 2 hours.  The code might seem a little weird,
    but it works, and that's what I'm interested in.  Code completion is
    written in C++, although GTK+ is written in standard C.  Should you think
    this is a problem, feel free to rewrite the code in C (it could be at
    least 4 times bigger).

    It uses some static data (I know, I'm a too lasy programmer to think about
    something better); this means that if you're having *two or more*
    GtkCompletionLine-s in a program, you're looking for trouble.  The static
    data will be *shared* between them, and completion might not work
    correctly.  However, I don't know for sure, and I'm not going to test
    this.

    Having all that said, you should know that I'm not actually a bad
    programmer ;-] The problem being too simple for huge code complications, I
    preferred the easy way of doing different kind of things.  It works quite
    fine, so "don't expect tons of C code for completion" (quoted from some
    sources in mini-commander applet of GNOME).


Compilation, installation
--------------------------

    Use the configure script:

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


Bugs
-----

    * It gets pretty slow...  Maybe I should consider writting it in ANSI-C,
      but.... maybe not.

    * Writting this README took me more time than writting the program.

    * As I mentioned before, the code is written in C++, although GTK+ is a
      C library.  This is not actually a bug; I like C++ because programs
      become clearer and easier to maintain, not to mention the source file
      size is smaller.  So, if anyone cares to port this to standard C, feel
      free to do it, but I fear the code should be rewritten (almost) from
      scratch.

    * Documentation is inexistent (except this file) (however, it would be
      quite useless).

    Should you have any problems mail me a detailed description; please put
    the text "ignore_me" in the subject line, for easy message filtering. :)
    (just kidding...  I would gladely help if I can).

    * Actually I worked more than 2 hours.  Anyway, the completion code took
      about 2-3 h to design and implement.


Disclaimer
-----------

    * The Short Way:
        NO WARRANTIES OF ANY KIND.  USE IT AT YOUR OWN RISK.

    * The Right Way:
        Please read the GNU General Public License.  This program falls under
        its terms.


                       (: END OF TERMS AND CONDITIONS :)
