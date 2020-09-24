/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Deviation is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "model.h"
#include "../music.h"
#include "extended_audio.h"
#include "tx.h"
#include "voice.h"

#define MATCH_SECTION(s) (strcasecmp(section, s) == 0)
#define MATCH_KEY(s)     (strcasecmp(name,    s) == 0)

#if HAS_EXTENDED_AUDIO

const char SECTION_VOICE_GLOBAL[] = "global";
const char SECTION_VOICE_CUSTOM[] = "custom";

typedef struct { u16 id; voice_parse_mode mode; u16 custom_linenum } ini_parse_data;

static int ini_handler(void* user, const char* section, const char* name, const char* value)
{
    (void) section;
    ini_parse_data* userdata = (ini_parse_data*)user;
    u16 req_id = userdata->id;
    u16 id = atoi(name);

    // find end of label (k) and get duration (text after k) as u16
    const char* ptr = value;
    u8 k = 0;
    u16 duration = 0;
    while (*ptr && *ptr != ',') {
        ptr++;
        k++;
    }
    if (*ptr == ',') {
        duration = atoi(ptr + 1);
    }

    if (userdata->mode == VOICE_PARSE_MATCH_CUSTOM_LINENUM) {
        // match the line number of a custom alert to simulate old behaviour of forming an unordered array from all entries
        if (MATCH_SECTION(SECTION_VOICE_CUSTOM)) {
            if (userdata->custom_linenum == req_id) {
                // old behaviour didn't bother to check the entry had label set, so don't replicate that here
                current_voice_mapping.id = id;
                current_voice_mapping.duration = duration;
                if (HAS_MUSIC_CONFIG)
                    strlcpy(tempstring, value, k+1);  // return a requested mp3 label passed at *user to tempstring        
            }
            userdata->custom_linenum++;
        }
        return 1;
    } else {
        if ( k && (req_id != MAX_VOICEMAP_ENTRIES) && (req_id == id) ) {
            current_voice_mapping.id = id;
            current_voice_mapping.duration = duration;
            if (HAS_MUSIC_CONFIG)
                strlcpy(tempstring, value, k+1);  // return a requested mp3 label passed at *user to tempstring
            return 1;
        }
        if (HAS_MUSIC_CONFIG) {
            if ( req_id == MAX_VOICEMAP_ENTRIES ) {
                if (MATCH_SECTION(SECTION_VOICE_GLOBAL)) {
                    Transmitter.voice_ini_entries = VOICE_INI_GLOBAL_ONLY;
                }
                if (MATCH_SECTION(SECTION_VOICE_CUSTOM)) {
                    // Store max entry number instead of count to ensure all are reachable in config
                    if ((id - CUSTOM_ALARM_ID + 1) > Transmitter.voice_ini_entries)
                        Transmitter.voice_ini_entries = id - CUSTOM_ALARM_ID + 1;
                    return 1;
                }
                return 0;
            }
        }
    }
    return 1;  // voice label ignored
}

const char* CONFIG_VoiceParse_WithMode(unsigned id, voice_parse_mode mode)
{
    #ifdef _DEVO12_TARGET_H_
    static char filename[] = "media/voice.ini\0\0\0"; // placeholder for longer folder name
    static u8 checked;
        if(!checked) {
            FILE *fh;
            fh = fopen("mymedia/voice.ini", "r");
            if(fh) {
                sprintf(filename, "mymedia/voice.ini");
                fclose(fh);
            }
            checked = 1;
        }
    #else
    char filename[] = "media/voice.ini";
    #endif

    ini_parse_data userdata = { .id = id, .mode = mode, .custom_linenum = 200 };
    if (id == MAX_VOICEMAP_ENTRIES) {  // initial parse of voice.ini
        userdata.mode = VOICE_PARSE_MATCH_ID;  // VOICE_PARSE_MATCH_CUSTOM_LINENUM not supported for initial parse
        if (CONFIG_IniParse(filename, ini_handler, &userdata))
            tempstring[0] = '\0';
        if (Transmitter.voice_ini_entries == VOICE_INI_EMPTY) {
            printf("Failed to parse voice.ini\n");
            Transmitter.audio_player = AUDIO_NONE;  // disable external voice when no global voices are found in voice.ini
        }
    }
    if ( (id < MAX_VOICEMAP_ENTRIES) ) {
        // reset vars
        current_voice_mapping.id = 0;
        current_voice_mapping.duration = 0;
        sprintf(tempstring, "%d", id);
        if (CONFIG_IniParse(filename, ini_handler, &userdata)) {
            // ini handler will return tempstring with label of id and fill current_voice_mapping
        }
        if (current_voice_mapping.duration == 0)
           strncat(tempstring, " (missing)", sizeof(tempstring) - strlen(tempstring) - 1);  // strlcat not available?
    }
    return tempstring;
}

const char* CONFIG_VoiceParse(unsigned id) {
    return CONFIG_VoiceParse_WithMode(id, VOICE_PARSE_MATCH_ID);
}
#endif
