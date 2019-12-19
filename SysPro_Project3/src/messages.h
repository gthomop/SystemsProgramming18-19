/**** messages.h ****/
#ifndef _MESSAGES_H
#define _MESSAGES_H

#define _OK                 (short) 0
#define _LOG_ON             (short) 1
#define _LOG_OFF            (short) 2
#define _GET_CLIENTS        (short) 3
#define _GET_FILE_LIST      (short) 4
#define _GET_FILE           (short) 5
#define _USER_OFF           (short) 6
#define _USER_ON            (short) 7
#define _FILE_LIST          (short) 8
#define _FILE_NOT_FOUND     (short) 9
#define _FILE_UP_TO_DATE    (short) 10
#define _CLIENT_LIST        (short) 11
#define _FILE_SIZE          (short) 12
#define _IP_PORT_NOT_FOUND  (short) 13
#define _SUCC_LOG_OFF       (short) 14

short OK                    = _OK;
short LOG_ON                = _LOG_ON;
short LOG_OFF               = _LOG_OFF;
short GET_CLIENTS           = _GET_CLIENTS;
short GET_FILE_LIST         = _GET_FILE_LIST;
short GET_FILE              = _GET_FILE;
short USER_OFF              = _USER_OFF;
short USER_ON               = _USER_ON;
short FILE_LIST             = _FILE_LIST;
short FILE_NOT_FOUND        = _FILE_NOT_FOUND;
short FILE_UP_TO_DATE       = _FILE_UP_TO_DATE;
short CLIENT_LIST           = _CLIENT_LIST;
short FILE_SIZE             = _FILE_SIZE;
short IP_PORT_NOT_FOUND     = _IP_PORT_NOT_FOUND;
short SUCC_LOG_OFF          = _SUCC_LOG_OFF;

#endif