# DCL exercises (ELTE, 2005)

Five practice tasks for DCL, the OpenVMS command language. They come from the
VMS/DCL programming assignments of the course *Programozási környezetek*
(Programming Environments) at Eötvös Loránd University (ELTE), Budapest, in the
2005/2006 autumn semester, taught by Ádám Maulis of ELTE's VMS Competence
Center.

The task descriptions below are written in our own words. For the original
Hungarian assignment page see [Sources](#sources), and for a former
student's English version with downloadable example solutions see
[Gergő Érdi's page](https://erdi.dev/elte/2005-1-vms/). Try the tasks yourself
before looking at his solutions.

DCL procedures are text files with the type `.COM`; run one with
`@NAME` and pass arguments after it (`@NAME arg1 arg2`, available inside as
`P1`, `P2`, ...). `HELP LEXICAL` lists the built-in `F$` functions you will need.

## 1. Multiplication table

Write `MULTIPLICATION.COM`, which prints the multiplication table up to N for a
number N given as the first argument. If a second argument M is given, print
every product modulo M instead.

    $ @MULTIPLICATION 4
     1  2  3  4
     2  4  6  8
     3  6  9 12
     4  8 12 16

Hints: `F$INTEGER`, `F$FAO` for aligned columns, `IF ... THEN GOTO` loops.

## 2. System statistics

Write `SYSSTAT.COM`, which runs `SHOW SYSTEM`, counts the processes, and prints
the average number of virtual-memory pages they use.

    $ @SYSSTAT
    15 processes, using 234 pages on average.

Hints: `SHOW SYSTEM/OUTPUT=file`, `OPEN`/`READ`/`CLOSE`, `F$ELEMENT` or
`F$EXTRACT` to pick columns. (Alternative: `F$PID` and `F$GETJPI` without
parsing text.)

## 3. File checksums

Write `MULTICHECK.COM` with two arguments: a file specification (wildcards
allowed) and the name of a checksum file.

- If the checksum file does not exist, compute a checksum for every matching
  file and store the results in it.
- If it exists, recompute the checksums and report every file that changed.

Hints: the `CHECKSUM` command and the symbol `CHECKSUM$CHECKSUM` it sets,
`F$SEARCH` to loop over wildcard matches, `F$SEARCH` again to test whether the
checksum file exists.

## 4. Comment removal

Write `UNCOMMENT.COM`, which copies a text file while removing comments. The
first argument is the input file, the second the comment-start string (for
example `!` or `//`); everything from that string to the end of a line goes.

Hints: `F$LOCATE`, `F$EXTRACT`, `F$LENGTH`, reading and writing files with
`OPEN/READ`, `OPEN/WRITE`, `READ`, `WRITE`.

## 5. A game

Write an interactive game in DCL: anything that reads the player's input with
`INQUIRE` or `READ SYS$COMMAND` and keeps state in symbols. Érdi's solution is a
one-player Battleship.

## Sources

- Original assignment page (Hungarian), no longer online and not in the
  Wayback Machine: `http://csirke.elte.hu/~maulis/progkorny/beadando15.html`.
  Earlier assignment sets from the same course (1998-2003) and its lecture
  notes are archived:
  [list of archived pages](https://web.archive.org/web/2006*/csirke.elte.hu/~maulis/progkorny/*)
- English translation and solutions by Gergő Érdi:
  https://erdi.dev/elte/2005-1-vms/
