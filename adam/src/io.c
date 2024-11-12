/**
 * Weather / io.c
 *
 * Based on @bocianu's code
 *
 * @author Norman Davie
 *
 */

#include <stdbool.h>
#include <eos.h>
#include <string.h>
#include <conio.h>
#include "constants.h"
#include "io.h"
#include "ftime.h"
#include "utils.h"
#include "weather.h"



#ifdef NO_FUJI

char dummy[1024];

unsigned char FAKE_eos_write_character_device(unsigned char dev, void *buf, unsigned short len)
{
    FUJI_CMD *fc = (FUJI_CMD *) buf;
    FUJI_JSON_QUERY *jq = (FUJI_JSON_QUERY *)buf;
    FUJI_TIME *ft = (FUJI_TIME *) dummy;
    FUJI_SET_CHANNEL *sc = (FUJI_SET_CHANNEL *)buf;
static bool firstTime = true;
static FUJI_TIME current;
static FUJI_TIME adjust;

    if ((dev == FUJI_DEV) || (dev == NET_DEV))
    {
        cprintf("write ");
        switch(dev)
        {
            case FUJI_DEV:
                cprintf("FUJI ");
                switch (fc->cmd)
                {
                    case 0xD2:
                        cprintf("Get Time");
                        if (firstTime)
                        {
                            firstTime = false;
                            
                            current.century = 20;
                            current.year = 24;
                            current.month = 6;
                            current.day = 29;
                            current.hour = 20;
                            current.minute = 50;
                            current.second = 00;

                            memset(&adjust, 0, sizeof(adjust));
                            adjust.second = 1;
                        } else
                        {
                            add_time(current, current, &adjust);
                        }
                        memcpy(dummy, &current, sizeof(FUJI_TIME));

                        break;
                    case 0xDB:
                        cprintf("Close App Key");
                        break;
                    case 0xDC:
                        cprintf("Open App Key");
                        break;
                    case 0xDD:
                        cprintf("Read App Key");
                        break;
                    case 0xDE:
                        cprintf("Write App Key");
                        break;
                    default:
                        cprintf("Unknown %02x\n", fc->cmd);
                        csleep(DEBUG_DELAY);
                }
                break;

            case NET_DEV:
                cprintf("NET  ");
                switch(fc->cmd)
                {
                    case 0xFC:
                        cprintf("Set Channel Mode ");
                        if (sc->mode == 0)
                            cprintf("PROTOCOL");
                        else
                            cprintf("JSON");
                        csleep(DEBUG_DELAY);
                        break;
                    case 'O':
                        cprintf("Open %s", fc->url);
                        csleep(DEBUG_DELAY);
                        break;
                    case 'C':
                        cprintf("Close");
                        csleep(DEBUG_DELAY);
                        break;
                    case 'P':
                        cprintf("Parse JSON");
                        csleep(DEBUG_DELAY);
                        break;
                    case 'Q':
                        cprintf("Query JSON '%s' \n", jq->query);
                        csleep(DEBUG_DELAY);
                        break;
                }
                break;
            default:
                break; 
        } // switch dev
        cprintf("\n");
        
        return ACK;
    } else
        eos_write_character_device(dev, buf, len);
}

unsigned char FAKE_eos_read_character_device(unsigned char dev, void *buf, unsigned short len)
{
    FUJI_CMD *fc = (FUJI_CMD *)buf;
    FUJI_JSON_QUERY *jq = (FUJI_JSON_QUERY *)buf;

    if ((dev == FUJI_DEV) || (dev == NET_DEV))
    {        
        memcpy(buf, dummy, sizeof(len));
        return ACK;
    } else
        eos_read_character_device(dev, buf, len);

}

#endif

unsigned char response[1024];

char *strncpy2(char *dest, char *src, size_t dest_size)
{
    char *p;

    p = strncpy(dest, src, dest_size-1);
    dest[dest_size-1] = '\0';
    return p;
}

char *io_get_next_string(char *start, char *result, int max_size)
{
    char *end;
    char *p;
    int i = 0;

    *result = '\0';

    if (start == NULL)
        return NULL;

    end = start;
    if ((*end == '\0') || (*end == '\n'))
        return NULL;

    while (! ((*end == '|') || (*end == '\n') || (*end == '\0')))
    {
        if (i >= max_size-1)
            break;

        *result = *end;
        result++;
        end++;
        i++;
    }
    *result = '\0';
    
    if ((*end == '|') || (*end == '\n'))
        end++;

    return end;
}

char *io_get_next_bool(char *start, bool *result)
{
    char temp[10];

    if (start == NULL)
        return NULL;

    char *p = io_get_next_string(start, temp, sizeof(temp));
    *result = (bool) (atoi(temp) != 0);
    return p;
}

char *io_get_next_int(char *start, int *result)
{
    char temp[10];

    if (start == NULL)
    {
        *result = 0;
        return NULL;
    }
    char *p = io_get_next_string(start, temp, sizeof(temp));
    *result = atoi(temp);
    return p;
}

char *io_get_next_long(char *start, long *result)
{
    char temp[10];

    if (start == NULL)
    {
        *result = 0L;
        return NULL;
    }   
    char *p = io_get_next_string(start, temp, sizeof(temp));
    *result = atol(temp);
    return p;
}


void debug_print(char *s)
{
    if (s == NULL)
        cprintf("NULL\n");
    else
        cprintf("%s\n",s);
    csleep(DEBUG_DELAY);
}

/*** CLOCK ****/

int io_time(FUJI_TIME *time)
{
 FUJI_CMD oc;

    unsigned char r = 0;

    oc.cmd = 0xD2; // Get Time

    // request time
    if (EOS_WRITE_CHARACTER_DEVICE(FUJI_DEV, (unsigned char *)oc, (unsigned short)sizeof(oc)) != ACK)
    {
        return 1; // could not open
    }

    r = EOS_READ_CHARACTER_DEVICE(FUJI_DEV, response, (unsigned short)sizeof(response));
    if (r != ACK)
    {
        return 3; // did not get result
    }

    memcpy(time, response, sizeof(time));
    return 0;
}

void add_time (FUJI_TIME *result, FUJI_TIME *time1, FUJI_TIME *add_time)
{
    memcpy(result, time1, sizeof(FUJI_TIME));

    result->hour   += add_time->hour;
    result->minute += add_time->minute;
    result->second += add_time->second;

    if (result->second > 59)
    {
        result->minute += (result->second / 60);
        result->second %= 60;
    }

    if (result->minute > 59)
    {
        result->hour += (result->minute / 60);
        result->minute %= 60;
    }

    if (result->hour > 23)
    {
        result->hour %= 24;  // we're not going into days.
    }

}

unsigned long time_in_seconds(FUJI_TIME *time) 
{ 
    return       time->second + 
          60   * time->minute + 
          3600 * time->hour; 
}

bool time_reached(FUJI_TIME *wait_until)
{
    FUJI_TIME  current;
    bool past_time = false;
    unsigned long   seconds_current;
    unsigned long   seconds_waiting;


    io_time(&current);

    // dealing with the case where the
    // future time has rolled to the
    // next day
    //    00:01               23:51
    if (wait_until->hour < current.hour)
        return false;

    seconds_current = time_in_seconds(&current);
    seconds_waiting = time_in_seconds(wait_until);

    return seconds_current >= seconds_waiting;

}



/*** JSON ****/


int io_json_open(char *url)
{
    FUJI_CMD oc;
    FUJI_SET_CHANNEL scm;

    oc.cmd = 'O';
    oc.mode = 12;
    oc.trans = 3;
    strncpy2(oc.url,url,sizeof(oc.url));

    scm.cmd = 0xFC;
    scm.mode = CHANNEL_MODE_JSON;

    // open url
    if (EOS_WRITE_CHARACTER_DEVICE(NET_DEV, (unsigned char *)oc, sizeof(oc)) != ACK)
    {
        return 1; // could not open
    }
  
    // set channel to json
    if (EOS_WRITE_CHARACTER_DEVICE(NET_DEV, (unsigned char *)scm, sizeof(scm)) != ACK)
    {
        return 2;  // could not set to json mode
    }
  
    // set json parsing mode
    if (EOS_WRITE_CHARACTER_DEVICE(NET_DEV, "P", 1) != ACK)
    {
        return 3; // could not set to parsing mode
    }

    return 0;
}

 int io_json_query(char *element, char *data, int max_buffer_size)
{
    FUJI_JSON_QUERY qcm;
    unsigned char r = 0;

    // zero out the response buffer
    memset(response, 0, sizeof(response));

    qcm.cmd = 'Q';
    strncpy2(qcm.query, element, sizeof(qcm.query));
    r = EOS_WRITE_CHARACTER_DEVICE(NET_DEV, (unsigned char *)qcm, sizeof(qcm));
    if (r != ACK)
    {
        return 1; // did not ack query command
    }

    r = EOS_READ_CHARACTER_DEVICE(NET_DEV, response, sizeof(response));
    if (r != ACK)
    {
        return 2; // did not get result
    }

    response[max_buffer_size-1] = '\0';
    memcpy(data, response, max_buffer_size);

    return 0;
}

int io_json_close(void)
{
    EOS_WRITE_CHARACTER_DEVICE(NET_DEV, "C", 1);
    return 0;
}

void io_decimals(char *d, int decimals)
{
    char *p = strchr(d, '.');
    if (p)
    {
        if (strlen(p) > decimals)
        {
            p += (decimals + 1);
            *p = '\0';
        }
    }
}
