/*
 *
 * Configuration file for ttc - Terminal Type Control -program.
 *
 * This file includes all possible configuration options for ttc and ttc 
 * daemon. Please read all documentation. We start configuring ttc first.
 *
 */

/*
 * =============================
 *
 *  ttc - Terminal Type Control
 *
 * =============================
 */

/*
 * Default option file is started if you give ttc without any arguments. 
 * Note that you can change this default here or you can give new options 
 * file from command line. Easier to modify it from here now!
 */
#define DEFOPTION "/etc/ttc.options"


/*
 * Default configuration file. This file includes all configurations
 * for ttc (authorization, login limits, etc.)
 */
#define CONF_FILE "/etc/ttc.config"


/*
 * Message what user will receive if he/she has authorization to get 
 * unlimited access.
 */
#define A_MESSAGE "Unlimited access granted."


/*
 * Message what user will receive when he/she tries to access that terminal 
 * type what was denied.
 */
#define D_MESSAGE "ILLEGAL TERMINAL TYPE! ACCESS DENIED!"


/* 
 * Message what user receive if his/her log limit for today is reached.
 */
#define LLMESSAGE \
"YOU'VE REACHED YOUR LOGIN LIMIT FOR TODAY!
YOU WILL BE ABLE TO LOGON BACK AGAIN TOMORROW!"


/* 
 * Message what user receive if his/her time limit is less than 1. I.e. 
 * it's zero and time limit is up.
 */
#define TLMESSAGE "YOU'VE ALREADY USED YOUR TIME LIMIT! CONNECTION WILL BE CLOSED!"


/* 
 * Message what user receive if his/her time limit file is faked or screwed 
 * up. However, normal user cannot edit his/her own time limit file.
 */
#define FMESSAGE "ILLEGAL TIME FILE TYPE! ACCESS DENIED!"



/*
 * =====================================
 *
 *  ttcd - Terminal Type Control daemon
 *
 * =====================================
 */

/* 
 * If you want, that ttc daemon will make debug messages then leave next 
 * line be. If you don't want or need any debug messages then remove  
 * comments from #undef DEBUG line and comment #define DEBUG line. If DEBUG 
 * is defined, all debug messages will be saved into /var/log/ttcdlog file.
 */
#define DEBUG
/* #undef DEBUG */


/* 
 * path for ttc daemon socket. This should suite for you too.
 */
#define SOCKET_PATH "/var/lock/"


/*
 * Minutes, what user is allowed to be logged on. After this time is up, 
 * session will be killed. This is the default time limit.
 */
#define A_TIME 60		/* MINUTES */


/* 
 * Minutes before next round to check if user is logged off and to add 
 * A_MINUTES to current time. 1 is minimum.
 */
#define A_MINUTES 1		/* MINUTES */


/* 
 * Seconds after killing message is printed, user will be killed.
 */
#define K_SECONDS 60		/* SECONDS */


/* 
 * Message that user receive after time is up. K_SECONDS after this user 
 * will be killed.
 */
#define MESSAGE "YOUR TIME IS UP! CONNECTION WILL BE CLOSED!"


/* 
 * Message what user receive when day is changing, and all time limit's 
 * will be going to set to zero, so that new day time limit could start.
 */
#define N_MESSAGE \
"SINCE DAY CHANCES IN COUPLE MINUTES, YOUR TODAY'S TIME LIMIT IS
UP IN 60 SECONDS. AT THE SAME TIME YOUR NEW TIME LIMIT BEGANS!!"


/* 
 * Message what user receive on timed tty's on every logon, if his/her 
 * time limit file exists. I.e. this message tell's how long user still can 
 * be logged on before his/her time limit is up. Do not remove '%d'!
 */
#define TIMELEFT "You have %d minutes to spare before your time limit is up."


/*
 * Message which tells user how many times he/she still can logon today 
 * before his/her log limit is reached. Do not remove '%d'!
 */
#define LOGLEFT "You can still logon %d times during today."
