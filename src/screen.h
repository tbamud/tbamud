/**
* @file screen.h
* Header file with ANSI color codes for online color.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*                                                                        
* All rights reserved.  See license for complete information.                                                                
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University 
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               
*/
#ifndef _SCREEN_H_
#define _SCREEN_H_

#define CNRM  "\x1B[0;0m"     /* "Normal"                            */ 
#define CNUL  ""              /* No Change                           */ 
#define KNRM  "\x1B[0m"       /* Foreground "Normal"                 */ 
#define KBLK  "\x1b[0;30m"    /* Foreground Black                    */ 
#define KRED  "\x1B[0;31m"    /* Foreground Dark Red                 */ 
#define KGRN  "\x1B[0;32m"    /* Foreground Dark Green               */ 
#define KYEL  "\x1B[0;33m"    /* Foreground Dark Yellow              */ 
#define KBLU  "\x1B[0;34m"    /* Foreground Dark Blue                */ 
#define KMAG  "\x1B[0;35m"    /* Foreground Dark Magenta             */ 
#define KCYN  "\x1B[0;36m"    /* Foreground Dark Cyan                */ 
#define KWHT  "\x1B[0;37m"    /* Foreground Dark White (Light Gray)  */ 
#define KNUL  ""              /* Foreground No Change                */ 
#define BBLK  "\x1B[1;30m"    /* Foreground Bright Black (Dark Gray) */ 
#define BRED  "\x1B[1;31m"    /* Foreground Bright Red               */ 
#define BGRN  "\x1B[1;32m"    /* Foreground Bright Green             */ 
#define BYEL  "\x1B[1;33m"    /* Foreground Bright Yellow            */ 
#define BBLU  "\x1B[1;34m"    /* Foreground Bright Blue              */ 
#define BMAG  "\x1B[1;35m"    /* Foreground Bright Magenta           */ 
#define BCYN  "\x1B[1;36m"    /* Foreground Bright Cyan              */ 
#define BWHT  "\x1B[1;37m"    /* Foreground Bright White             */ 

#define BKBLK  "\x1B[40m"     /* Background Black                    */ 
#define BKRED  "\x1B[41m"     /* Background Dark Red                 */ 
#define BKGRN  "\x1B[42m"     /* Background Dark Green               */ 
#define BKYEL  "\x1B[43m"     /* Background Dark Yellow              */ 
#define BKBLU  "\x1B[44m"     /* Background Dark Blue                */ 
#define BKMAG  "\x1B[45m"     /* Background Dark Magenta             */ 
#define BKCYN  "\x1B[46m"     /* Background Dark Cyan                */ 
#define BKWHT  "\x1B[47m"     /* Background Dark White (Light Gray)  */ 

#define FBLK  "\x1B[5;30m"    /* Foreground Flashing Black (silly)   */ 
#define FRED  "\x1B[5;31m"    /* Foreground Flashing Dark Red        */ 
#define FGRN  "\x1B[5;32m"    /* Foreground Flashing Dark Green      */ 
#define FYEL  "\x1B[5;33m"    /* Foreground Flashing Dark Yellow     */ 
#define FBLU  "\x1B[5;34m"    /* Foreground Flashing Dark Blue       */ 
#define FMAG  "\x1B[5;35m"    /* Foreground Flashing Dark Magenta    */ 
#define FCYN  "\x1B[5;36m"    /* Foreground Flashing Dark Cyan       */ 
#define FWHT  "\x1B[5;37m"    /* Foreground Flashing Light Gray      */ 

#define BFBLK  "\x1B[1;5;30m" /* Foreground Flashing Dark Gray       */ 
#define BFRED  "\x1B[1;5;31m" /* Foreground Flashing Bright Red      */ 
#define BFGRN  "\x1B[1;5;32m" /* Foreground Flashing Bright Green    */ 
#define BFYEL  "\x1B[1;5;33m" /* Foreground Flashing Bright Yellow   */ 
#define BFBLU  "\x1B[1;5;34m" /* Foreground Flashing Bright Blue     */ 
#define BFMAG  "\x1B[1;5;35m" /* Foreground Flashing Bright Magenta  */ 
#define BFCYN  "\x1B[1;5;36m" /* Foreground Flashing Bright Cyan     */ 
#define BFWHT  "\x1B[1;5;37m" /* Foreground Flashing Bright White    */ 

#define CBEEP "\x07" 
#define CAT   "@@"
#define CAMP  "&" 
#define CSLH  "\\" 

#define CUDL  "\x1B[4m" /* Underline ANSI code */ 
#define CFSH  "\x1B[5m" /* Flashing ANSI code.  Change to #define CFSH "" if 
                         * you want to disable flashing colour codes 
                         */ 
#define CRVS  "\x1B[7m" /* Reverse video ANSI code */ 

/* conditional color.  pass it a pointer to a char_data and a color level. */ 
#define C_OFF   0 
#define C_SPR   1 
#define C_NRM   2 
#define C_CMP   3 
#define _clrlevel(ch) (!IS_NPC(ch) ? (PRF_FLAGGED((ch), PRF_COLOR_1) ? 1 : 0) + \
			(PRF_FLAGGED((ch), PRF_COLOR_2) ? 2 : 0) : 0) 
#define clr(ch,lvl) (_clrlevel(ch) >= (lvl)) 

/* Player dependant foreground color codes */ 
#define CCNRM(ch,lvl)  (clr((ch),(lvl))?KNRM:KNUL) 
#define CCBLK(ch,lvl)  (clr((ch),(lvl))?KBLK:CNUL) 
#define CCRED(ch,lvl)  (clr((ch),(lvl))?KRED:KNUL) 
#define CCGRN(ch,lvl)  (clr((ch),(lvl))?KGRN:KNUL) 
#define CCYEL(ch,lvl)  (clr((ch),(lvl))?KYEL:KNUL) 
#define CCBLU(ch,lvl)  (clr((ch),(lvl))?KBLU:KNUL) 
#define CCMAG(ch,lvl)  (clr((ch),(lvl))?KMAG:KNUL) 
#define CCCYN(ch,lvl)  (clr((ch),(lvl))?KCYN:KNUL) 
#define CCWHT(ch,lvl)  (clr((ch),(lvl))?KWHT:KNUL) 

/* Bright colors */ 
#define CBRED(ch,lvl)  (clr((ch),(lvl))?BRED:CNUL) 
#define CBGRN(ch,lvl)  (clr((ch),(lvl))?BGRN:CNUL) 
#define CBYEL(ch,lvl)  (clr((ch),(lvl))?BYEL:CNUL) 
#define CBBLU(ch,lvl)  (clr((ch),(lvl))?BBLU:CNUL) 
#define CBMAG(ch,lvl)  (clr((ch),(lvl))?BMAG:CNUL) 
#define CBCYN(ch,lvl)  (clr((ch),(lvl))?BCYN:CNUL) 
#define CBWHT(ch,lvl)  (clr((ch),(lvl))?BWHT:CNUL) 
#define CBBLK(ch,lvl)  (clr((ch),(lvl))?BBLK:CNUL) 

/* Flashing colors */ 
#define CCFRED(ch,lvl)  (clr((ch),(lvl))?FRED:CNUL) 
#define CCFGRN(ch,lvl)  (clr((ch),(lvl))?FGRN:CNUL) 
#define CCFYEL(ch,lvl)  (clr((ch),(lvl))?FYEL:CNUL) 
#define CCFBLU(ch,lvl)  (clr((ch),(lvl))?FBLU:CNUL) 
#define CCFMAG(ch,lvl)  (clr((ch),(lvl))?FMAG:CNUL) 
#define CCFCYN(ch,lvl)  (clr((ch),(lvl))?FCYN:CNUL) 
#define CCFWHT(ch,lvl)  (clr((ch),(lvl))?FWHT:CNUL) 

/* Flashing bright colors */ 
#define CBFRED(ch,lvl)  (clr((ch),(lvl))?BFRED:CNUL) 
#define CBFGRN(ch,lvl)  (clr((ch),(lvl))?BFGRN:CNUL) 
#define CBFYEL(ch,lvl)  (clr((ch),(lvl))?BFYEL:CNUL) 
#define CBFBLU(ch,lvl)  (clr((ch),(lvl))?BFBLU:CNUL) 
#define CBFMAG(ch,lvl)  (clr((ch),(lvl))?BFMAG:CNUL) 
#define CBFCYN(ch,lvl)  (clr((ch),(lvl))?BFCYN:CNUL) 
#define CBFWHT(ch,lvl)  (clr((ch),(lvl))?BFWHT:CNUL) 

/* Background colors */ 
#define CBKRED(ch,lvl)  (clr((ch),(lvl))?BKRED:CNUL) 
#define CBKGRN(ch,lvl)  (clr((ch),(lvl))?BKGRN:CNUL) 
#define CBKYEL(ch,lvl)  (clr((ch),(lvl))?BKYEL:CNUL) 
#define CBKBLU(ch,lvl)  (clr((ch),(lvl))?BKBLU:CNUL) 
#define CBKMAG(ch,lvl)  (clr((ch),(lvl))?BKMAG:CNUL) 
#define CBKCYN(ch,lvl)  (clr((ch),(lvl))?BKCYN:CNUL) 
#define CBKWHT(ch,lvl)  (clr((ch),(lvl))?BKWHT:CNUL) 
#define CBKBLK(ch,lvl)  (clr((ch),(lvl))?BKBLK:CNUL) 

#define COLOR_LEV(ch) (_clrlevel(ch)) 

/* Simplified color codes */ 
#define QNRM CCNRM(ch,C_SPR) 
#define QBLK CCBLK(ch,C_SPR) 
#define QRED CCRED(ch,C_SPR) 
#define QGRN CCGRN(ch,C_SPR) 
#define QYEL CCYEL(ch,C_SPR) 
#define QBLU CCBLU(ch,C_SPR) 
#define QMAG CCMAG(ch,C_SPR) 
#define QCYN CCCYN(ch,C_SPR) 
#define QWHT CCWHT(ch,C_SPR) 

/* simplified brights */ 
#define QBBLK CBBLK(ch,C_SPR) 
#define QBRED CBRED(ch,C_SPR) 
#define QBGRN CBGRN(ch,C_SPR) 
#define QBYEL CBYEL(ch,C_SPR) 
#define QBBLU CBBLU(ch,C_SPR) 
#define QBMAG CBMAG(ch,C_SPR) 
#define QBCYN CBCYN(ch,C_SPR) 
#define QBWHT CBWHT(ch,C_SPR) 

/* Simplified Flashing */ 
#define QFRED CCFRED(ch,C_SPR) 
#define QFGRN CCFGRN(ch,C_SPR) 
#define QFYEL CCFYEL(ch,C_SPR) 
#define QFBLU CCFBLU(ch,C_SPR) 
#define QFMAG CCFMAG(ch,C_SPR) 
#define QFCYN CCFCYN(ch,C_SPR) 
#define QFWHT CCFWHT(ch,C_SPR) 

/* Simplified Bright Flashing */ 
#define QBFRED CBFRED(ch,C_SPR) 
#define QBFGRN CBFGRN(ch,C_SPR) 
#define QBFYEL CBFYEL(ch,C_SPR) 
#define QBFBLU CBFBLU(ch,C_SPR) 
#define QBFMAG CBFMAG(ch,C_SPR) 
#define QBFCYN CBFCYN(ch,C_SPR) 
#define QBFWHT CBFWHT(ch,C_SPR) 

/* Simplified Backgrounds */ 
#define QBKBLK CBKBLK(ch,C_SPR) 
#define QBKRED CBKRED(ch,C_SPR) 
#define QBKGRN CBKGRN(ch,C_SPR) 
#define QBKYEL CBKYEL(ch,C_SPR) 
#define QBKBLU CBKBLU(ch,C_SPR) 
#define QBKMAG CBKMAG(ch,C_SPR) 
#define QBKCYN CBKCYN(ch,C_SPR) 
#define QBKWHT CBKWHT(ch,C_SPR) 

#endif /* _SCREEN_H_ */
