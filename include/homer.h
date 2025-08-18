/**
 * @file homer.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#define PANIC_BANNER setforeground(COLOUR_LIGHTYELLOW); \
      kprintf("\n\
              ___  _____ \n\
            .'/,-Y\"     \"~-. \n\
            l.Y             ^. \n\
            /\\               _\\_      \"Oh\n\
           i            ___/\"   \"\\     so they have internet on computers now!\"\n\
           |          /\"   \"\\   o !    \n\
           l         ]     o !__./     \n\
            \\ _  _    \\.___./    \"~\\ \n\
             X \\/ \\            ___./ \n\
            ( \\ ___.   _..--~~\"   ~`-. \n\
             ` Z,--   /               \\ \n\
               \\__.  (   /       ______) \n\
                 \\   l  /-----~~\" / \n\
                  Y   \\          /\n\
                  |    \"x______.^ \n\
                  |           \\ \n\
\n"); setforeground(COLOUR_LIGHTWHITE);\
kprintf("This is a fatal system error and your system has been halted.\n\
"); setforeground(COLOUR_LIGHTRED);

