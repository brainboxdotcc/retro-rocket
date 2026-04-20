hljs.registerLanguage('rrbasic', function(hljs) {
    const keyword_list = [
        '[',
        ']',
        'ARRSORT',
        'ARRSORTBY',
        'REM',
        'LET',
        'PRINT',
        'IF',
        'THEN',
        'ELSE',
        'CHAIN',
        'FOR',
        'STEP',
        'TO',
        'NEXT',
        'CURSOR',
        'GOTO',
        'GOSUB',
        'RETURN',
        'CALL',
        'INPUT',
        'COLOUR',
        'PAGE',
        'FADE',
        'SPACE',
        'COLOR',
        'BACKGROUND',
        'EVAL',
        'CLOSE',
        'DEF',
        'PROC',
        'ENDPROC',
        'FN',
        'END',
        'AND',
        'OR',
        'NOT',
        'EOR',
        'MOD',
        'GLOBAL',
        'SOCKREAD',
        'SOCKWRITE',
        'CONNECT',
        'SOCKCLOSE',
        'CLS',
        'GCOL',
        'LINE',
        'TRIANGLE',
        'RECTANGLE',
        'CIRCLE',
        'POINT',
        'DATA',
        'RESTORE',
        'WRITE',
        'MKDIR',
        'RMDIR',
        'DELETE',
        'REPEAT',
        'UNTIL',
        'DIM',
        'REDIM',
        'PUSH',
        'POKE',
        'POKEW',
        'POKED',
        'POKEQ',
        'POP',
        'LOCAL',
        'CHDIR',
        'LIBRARY',
        'YIELD',
        'SETVARI',
        'SETVARR',
        'SETVARS',
        'SPRITELOAD',
        'SPRITEFREE',
        'PLOT',
        'AUTOFLIP',
        'FLIP',
        'KEYMAP',
        'MOUNT',
        'SETTIMEZONE',
        'ENDIF',
        'PLOTQUAD',
        'ON',
        'OFF',
        'WHILE',
        'ENDWHILE',
        'SLEEP',
        'CONTINUE',
        'UDPBIND',
        'UDPUNBIND',
        'UDPWRITE',
        'OUTPORT',
        'OUTPORTW',
        'OUTPORTD',
        'KGET',
        'MODLOAD',
        'MODUNLOAD',
        'SOCKFLUSH',
        'MEMRELEASE',
        'SOCKBINWRITE',
        'SOCKBINREAD',
        'BINREAD',
        'BINWRITE',
        'GRAPHPRINT',
        'VDU',
        'STREAM',
        'CREATE',
        'DESTROY',
        'SOUND',
        'VOLUME',
        'PLAY',
        'STOP',
        'PAUSE',
        'LOAD',
        'UNLOAD',
        'ENVELOPE',
        'TONE',
        'SCROLLREGION',
        'SSLCONNECT',
        'ANIMATE',
        'RESET',
        'MATCH',
        'DATASET',
        'ROTATE',
        'SPRITEROW',
        'ARRAYFIND',
        'MAPSET',
    ];

    const literal_list = [
        'TRUE',
        'FALSE',
        'PI#',
        'E#',
        'ARGS$',
        'PID',
    ];

    const built_in_list = [
        'ABS',
        'ALTKEY',
        'ASC',
        'BOOL$',
        'BITAND',
        'BITNAND',
        'BITNOR',
        'BITNOT',
        'BITOR',
        'BITROL',
        'BITROR',
        'BITSHL',
        'BITSHR',
        'BITEOR',
        'BITXNOR',
        'CAPSLOCK',
        'CHR$',
        'COS',
        'CPUGETBRAND$',
        'CPUGETVENDOR$',
        'CPUID',
        'CTRLKEY',
        'CURRENTX',
        'CURRENTY',
        'CSD$',
        'DATE$',
        'DATAREAD',
        'DATAREAD$',
        'DATAREADR',
        'DAY',
        'DECIBELS',
        'DEG',
        'DNS$',
        'MAPGET',
        'MAPGET$',
        'EMPTYRAMDISK$',
        'EOF',
        'EPOCH',
        'EXISTSVARI',
        'EXISTSVARR',
        'EXISTSVARS',
        'EXP',
        'FADE',
        'SPACE',
        'FILESIZE',
        'FILETYPE$',
        'FMOD',
        'GETNAME$',
        'GETNAMECOUNT',
        'GETPROCCOUNT',
        'GETPROCCPUID',
        'GETPROCID',
        'GETPROCMEM',
        'GETPROCNAME$',
        'GETPROCPARENT',
        'GETPROCSTATE$',
        'GETSIZE',
        'GETVARI',
        'GETVARR',
        'GETVARS$',
        'GRAPHICS_CENTRE_X',
        'GRAPHICS_CENTRE_Y',
        'GRAPHICS_HEIGHT',
        'GRAPHICS_WIDTH',
        'HEXVAL',
        'HIGHLIGHT$',
        'HOUR',
        'INKEY$',
        'INPORT',
        'INPORTD',
        'INPORTW',
        'INSOCKET$',
        'INT',
        'INSTR',
        'INTOASC$',
        'LCPUID',
        'LEFT$',
        'LEN',
        'LGETLASTCPUID',
        'LJUST$',
        'LOG',
        'LOWER$',
        'LTRIM$',
        'MEMALLOC',
        'MEMFREE',
        'MEMORY',
        'MEMPEAK',
        'MEMPROGRAM',
        'MEMUSED',
        'MID$',
        'MINUTE',
        'MONTH',
        'NETINFO$',
        'OCTVAL',
        'OPENIN',
        'OPENOUT',
        'OPENUP',
        'PATH$',
        'PEEK',
        'PEEKD',
        'PEEKQ',
        'PEEKW',
        'POW',
        'PROGRAM$',
        'RAD',
        'RADIX',
        'RADIX$',
        'RAMDISK$',
        'READ',
        'READ$',
        'REALVAL',
        'REPLACE$',
        'REP$',
        'REVERSE$',
        'RIGHT$',
        'RJUST$',
        'RGB',
        'RND',
        'ROUND',
        'RTRIM$',
        'SECOND',
        'SGN',
        'SHIFTKEY',
        'SHL',
        'SHR',
        'SIN',
        'SOCKACCEPT',
        'SOCKLISTEN',
        'SOCKSTATUS',
        'SPRITECOLLIDE',
        'SPRITEHEIGHT',
        'SPRITEWIDTH',
        'SQR',
        'SQRT',
        'SSLSOCKACCEPT',
        'STR$',
        'TAN',
        'TERMHEIGHT',
        'TERMWIDTH',
        'TIME$',
        'TOKENIZE$',
        'TRIM$',
        'UDPLASTIP$',
        'UDPLASTIP',
        'UDPLASTSOURCEPORT',
        'UDPREAD$',
        'UPPER$',
        'UPSECS',
        'UPTIME$',
        'VAL',
        'WEEKDAY',
        'YDAY',
        'MAPHAS',
        'YEAR',
        'ARGS$',
        'ERR$',
        'LIB$',
        'TLSCIPHER$',
        'TLSVERSION$',
    ];

    return {
        name: 'Retro Rocket BASIC',
        aliases: ['rrbasic'],
        case_insensitive: false,
        keywords: {
            keyword: keyword_list.join(' '),
            literal: literal_list.join(' '),
            built_in: built_in_list.join(' ')
        },
        contains: [
            {
                className: 'comment',
                begin: /^\s*REM\b/,
                end: /$/
            },
            {
                className: 'comment',
                begin: /'/,
                end: /$/
            },
            {
                className: 'string',
                begin: /"/,
                end: /"/,
                contains: [{ begin: /""/ }]
            },
            {
                className: 'number',
                begin: /(?:\b\d+(?:\.\d+)?|\&[0-9A-F]+)\b/i
            },
            {
                className: 'title.function',
                begin: /\bPROC[A-Z_][A-Z0-9_$#]*/
            },
            {
                className: 'title.function',
                begin: /\bFN[A-Z_][A-Z0-9_$#]*/
            },
            {
                className: 'variable',
                begin: /\b[A-Za-z_][A-Za-z0-9_]*[$#]?/
            },
        ]
    };
});

function detect_language(text) {
    var rrbasic_score = 0;
    var c_score = 0;

    const tokens = new Set([
        // Core tokens
        "[", "]", "PRINT","IF","THEN","ELSE","CHAIN","FOR","STEP","TO","NEXT",
        "CURSOR","GOTO","GOSUB","RETURN","INPUT","COLOUR","COLOR",
        "BACKGROUND","EVAL","CLOSE","DEF","PROC","ENDPROC","FN","END",
        "REM","AND","OR","NOT","EOR","MOD","GLOBAL","CLS","GCOL",
        "LINE","TRIANGLE","RECTANGLE","CIRCLE","POINT","DATA","RESTORE",
        "WRITE","MKDIR","RMDIR","DELETE","REPEAT","UNTIL","DIM","REDIM",
        "PUSH","POKE","POP","LOCAL","CHDIR","LIBRARY","YIELD","SETVARI",
        "SETVARR","SETVARS","SPRITELOAD","SPRITEFREE","PLOT","AUTOFLIP",
        "FLIP","KEYMAP","MOUNT","SETTIMEZONE","ENDIF","PLOTQUAD","ON",
        "OFF","WHILE","ENDWHILE","SLEEP","CONTINUE","MODLOAD","MODUNLOAD",
        "STREAM","CREATE","DESTROY","SOUND","PLAY","STOP","LOAD","UNLOAD",
        "ROTATE", "SPRITEROW", "ARRAYFIND", "ARRSORT", "ARRSORTBY", "MAPSET",
    ]);

    const builtins = new Set([
        // int
        "ABS","ASC","CTRLKEY","EOF","EXISTSVARI","GETVARI","LEN","RND",
        "SOCKACCEPT","SOCKLISTEN","SOCKSTATUS","TERMHEIGHT","TERMWIDTH",
        "YEAR","INPORT","INPORTW","INPORTD","MEMFREE","FILESIZE",
        "SPRITEWIDTH","SPRITEHEIGHT","DATAREAD", "MAPGET", "MAPHAS",

        // double
        "COS","SIN","TAN","SQRT","SQR","ATAN","EXP","LOG",

        // string
        "CHR$","INKEY$","LEFT$","RIGHT$","MID$","REPLACE$","LOWER$",
        "UPPER$","READ$","STR$","TIME$","DATE$","DNS$","NETINFO$",
        "MAPGET$","TLSVERSION$","TLSCIPHER$"
    ]);

    // Tokenise
    var words = text.toUpperCase().match(/[A-Z_][A-Z0-9_$]*/g) || [];

    words.forEach(function(w) {
        if (tokens.has(w)) {
            rrbasic_score += 3;
        }
        if (builtins.has(w)) {
            rrbasic_score += 2;
        }
        if (w.endsWith("$")) {
            rrbasic_score += 2;
        }
    });

    // Strong RR BASIC structure
    if (/^\s*\[/im.test(text)) rrbasic_score += 8;
    if (/^\s*REM\b/im.test(text)) rrbasic_score += 6;
    if (/\bDEF\s+(PROC|FN)\b/i.test(text)) rrbasic_score += 8;
    if (/\bENDPROC\b/i.test(text)) rrbasic_score += 6;
    if (/\bREPEAT\b[\s\S]*\bUNTIL\b/i.test(text)) rrbasic_score += 6;
    if (/\b<>\b/.test(text)) rrbasic_score += 4;

    // C detection
    if (/^\s*#\s*include\b/m.test(text)) c_score += 8;
    if (/typedef\s+struct/.test(text)) c_score += 8;
    if (/struct\s+\w+\s*\{/.test(text)) c_score += 6;
    if (/\{[\s\S]*\}/.test(text)) c_score += 4;
    if (/;\s*$/m.test(text)) c_score += 3;
    if (/\b(int|char|void|size_t|uint\d+_t|int\d+_t)\b/.test(text)) c_score += 4;
    if (/->|\+\+|--/.test(text)) c_score += 4;
    if (/\/\*|\/\//.test(text)) c_score += 3;

    // Decision
    if (rrbasic_score >= c_score && rrbasic_score >= 6) {
        return "rrbasic";
    }

    if (c_score > rrbasic_score && c_score >= 6) {
        return "cpp";
    }

    return "text";
}