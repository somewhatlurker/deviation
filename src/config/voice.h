#ifndef _VOICE_H_
#define _VOICE_H_

// VOICE_PARSE_MATCH_CUSTOM_LINENUM looks up what the old voice system would have in a given voice_map entry
typedef enum { VOICE_PARSE_MATCH_ID, VOICE_PARSE_MATCH_CUSTOM_LINENUM } voice_parse_mode;

const char* CONFIG_VoiceParse_WithMode(unsigned id, voice_parse_mode mode);
const char* CONFIG_VoiceParse(unsigned id);

#endif
