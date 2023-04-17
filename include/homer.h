/**
 * @file homer.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2023
 */
#pragma once

#define PANIC_BANNER setforeground(current_console, COLOUR_LIGHTYELLOW); \
      kprintf("\n\
              ___  _____ \n\
            .'/,-Y\"     \"~-. \n\
            l.Y             ^. \n\
            /\\               _\\_      \"Yaaaaah! oh-my-GOD!\n\
           i            ___/\"   \"\\     We're trapped inside some\n\
           |          /\"   \"\\   o !    computer-dimension-\n\
           l         ]     o !__./     something-or-another!\"\n\
            \\ _  _    \\.___./    \"~\\ \n\
             X \\/ \\            ___./ \n\
            ( \\ ___.   _..--~~\"   ~`-. \n\
             ` Z,--   /               \\ \n\
               \\__.  (   /       ______) \n\
                 \\   l  /-----~~\" / \n\
                  Y   \\          /\n\
                  |    \"x______.^ \n\
                  |           \\ \n\
\n"); setforeground(current_console, COLOUR_LIGHTWHITE);\
kprintf("This is a fatal system error and your system has been halted.\n\
"); setforeground(current_console, COLOUR_LIGHTRED);

