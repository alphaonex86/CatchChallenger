#include "../../general/base/GeneralVariable.h"

#if defined(CATCHCHALLENGER_CLIENT)
#ifndef CACHEDSTRING_H
#define CACHEDSTRING_H

#if defined(CATCHCHALLENGER_NATIVEMODESTDSTRING)

#define XMLCACHEDSTRING_yes CACHEDSTRING_yes
#define XMLCACHEDSTRING_no CACHEDSTRING_no
#define XMLCACHEDSTRING_type CACHEDSTRING_type
#define XMLCACHEDSTRING_name CACHEDSTRING_name
#define XMLCACHEDSTRING_id CACHEDSTRING_id
#define XMLCACHEDSTRING_multiplicator CACHEDSTRING_multiplicator
#define XMLCACHEDSTRING_number CACHEDSTRING_number
#define XMLCACHEDSTRING_to CACHEDSTRING_to
#define XMLCACHEDSTRING_dotcoma CACHEDSTRING_dotcoma
#define XMLCACHEDSTRING_list CACHEDSTRING_list
#define XMLCACHEDSTRING_monster CACHEDSTRING_monster
#define XMLCACHEDSTRING_monsters CACHEDSTRING_monsters
#define XMLCACHEDSTRING_dotxml CACHEDSTRING_dotxml
#define XMLCACHEDSTRING_skills CACHEDSTRING_skills
#define XMLCACHEDSTRING_buffs CACHEDSTRING_buffs
#define XMLCACHEDSTRING_egg_step CACHEDSTRING_egg_step
#define XMLCACHEDSTRING_xp_for_max_level CACHEDSTRING_xp_for_max_level
#define XMLCACHEDSTRING_xp_max CACHEDSTRING_xp_max
#define XMLCACHEDSTRING_hp CACHEDSTRING_hp
#define XMLCACHEDSTRING_attack CACHEDSTRING_attack
#define XMLCACHEDSTRING_defense CACHEDSTRING_defense
#define XMLCACHEDSTRING_special_attack CACHEDSTRING_special_attack
#define XMLCACHEDSTRING_special_defense CACHEDSTRING_special_defense
#define XMLCACHEDSTRING_speed CACHEDSTRING_speed
#define XMLCACHEDSTRING_give_sp CACHEDSTRING_give_sp
#define XMLCACHEDSTRING_give_xp CACHEDSTRING_give_xp
#define XMLCACHEDSTRING_catch_rate CACHEDSTRING_catch_rate
#define XMLCACHEDSTRING_type2 CACHEDSTRING_type2
#define XMLCACHEDSTRING_pow CACHEDSTRING_pow
#define XMLCACHEDSTRING_ratio_gender CACHEDSTRING_ratio_gender
#define XMLCACHEDSTRING_percent CACHEDSTRING_percent
#define XMLCACHEDSTRING_attack_list CACHEDSTRING_attack_list
#define XMLCACHEDSTRING_skill CACHEDSTRING_skill
#define XMLCACHEDSTRING_skill_level CACHEDSTRING_skill_level
#define XMLCACHEDSTRING_attack_level CACHEDSTRING_attack_level
#define XMLCACHEDSTRING_level CACHEDSTRING_level
#define XMLCACHEDSTRING_byitem CACHEDSTRING_byitem
#define XMLCACHEDSTRING_evolution CACHEDSTRING_evolution
#define XMLCACHEDSTRING_evolutions CACHEDSTRING_evolutions
#define XMLCACHEDSTRING_trade CACHEDSTRING_trade
#define XMLCACHEDSTRING_evolveTo CACHEDSTRING_evolveTo
#define XMLCACHEDSTRING_item CACHEDSTRING_item
#define XMLCACHEDSTRING_fights CACHEDSTRING_fights
#define XMLCACHEDSTRING_fight CACHEDSTRING_fight
#define XMLCACHEDSTRING_gain CACHEDSTRING_gain
#define XMLCACHEDSTRING_cash CACHEDSTRING_cash
#define XMLCACHEDSTRING_sp CACHEDSTRING_sp
#define XMLCACHEDSTRING_effect CACHEDSTRING_effect
#define XMLCACHEDSTRING_endurance CACHEDSTRING_endurance
#define XMLCACHEDSTRING_life CACHEDSTRING_life
#define XMLCACHEDSTRING_applyOn CACHEDSTRING_applyOn
#define XMLCACHEDSTRING_quantity CACHEDSTRING_quantity
#define XMLCACHEDSTRING_more CACHEDSTRING_more
#define XMLCACHEDSTRING_success CACHEDSTRING_success
#define XMLCACHEDSTRING_buff CACHEDSTRING_buff
#define XMLCACHEDSTRING_capture_bonus CACHEDSTRING_capture_bonus
#define XMLCACHEDSTRING_duration CACHEDSTRING_duration
#define XMLCACHEDSTRING_Always CACHEDSTRING_Always
#define XMLCACHEDSTRING_NumberOfTurn CACHEDSTRING_NumberOfTurn
#define XMLCACHEDSTRING_durationNumberOfTurn CACHEDSTRING_durationNumberOfTurn
#define XMLCACHEDSTRING_ThisFight CACHEDSTRING_ThisFight
#define XMLCACHEDSTRING_inFight CACHEDSTRING_inFight
#define XMLCACHEDSTRING_inWalk CACHEDSTRING_inWalk
#define XMLCACHEDSTRING_steps CACHEDSTRING_steps
#define XMLCACHEDSTRING_nobody CACHEDSTRING_nobody
#define XMLCACHEDSTRING_allAlly CACHEDSTRING_allAlly
#define XMLCACHEDSTRING_allEnemy CACHEDSTRING_allEnemy
#define XMLCACHEDSTRING_themself CACHEDSTRING_themself
#define XMLCACHEDSTRING_aloneEnemy CACHEDSTRING_aloneEnemy
#define XMLCACHEDSTRING_width CACHEDSTRING_width
#define XMLCACHEDSTRING_height CACHEDSTRING_height
#define XMLCACHEDSTRING_properties CACHEDSTRING_properties
#define XMLCACHEDSTRING_property CACHEDSTRING_property
#define XMLCACHEDSTRING_value CACHEDSTRING_value
#define XMLCACHEDSTRING_tilewidth CACHEDSTRING_tilewidth
#define XMLCACHEDSTRING_tileheight CACHEDSTRING_tileheight
#define XMLCACHEDSTRING_objectgroup CACHEDSTRING_objectgroup
#define XMLCACHEDSTRING_object CACHEDSTRING_object
#define XMLCACHEDSTRING_x CACHEDSTRING_x
#define XMLCACHEDSTRING_y CACHEDSTRING_y
#define XMLCACHEDSTRING_layer CACHEDSTRING_layer
#define XMLCACHEDSTRING_data CACHEDSTRING_data
#define XMLCACHEDSTRING_encoding CACHEDSTRING_encoding
#define XMLCACHEDSTRING_compression CACHEDSTRING_compression
#define XMLCACHEDSTRING_base64 CACHEDSTRING_base64
#define XMLCACHEDSTRING_zlib CACHEDSTRING_zlib
#define XMLCACHEDSTRING_zstd CACHEDSTRING_zstd
#define XMLCACHEDSTRING_minLevel CACHEDSTRING_minLevel
#define XMLCACHEDSTRING_maxLevel CACHEDSTRING_maxLevel
#define XMLCACHEDSTRING_luck CACHEDSTRING_luck
#define XMLCACHEDSTRING_condition CACHEDSTRING_condition
#define XMLCACHEDSTRING_quest CACHEDSTRING_quest
#define XMLCACHEDSTRING_fightBot CACHEDSTRING_fightBot
#define XMLCACHEDSTRING_zone CACHEDSTRING_zone
#define XMLCACHEDSTRING_capture CACHEDSTRING_capture
#define XMLCACHEDSTRING_fightId CACHEDSTRING_fightId
#define XMLCACHEDSTRING_shop CACHEDSTRING_shop
#define XMLCACHEDSTRING_fightid CACHEDSTRING_fightid
#define XMLCACHEDSTRING_drops CACHEDSTRING_drops
#define XMLCACHEDSTRING_drop CACHEDSTRING_drop
#define XMLCACHEDSTRING_quantity_min CACHEDSTRING_quantity_min
#define XMLCACHEDSTRING_quantity_max CACHEDSTRING_quantity_max

#else

#define XMLCACHEDSTRING_yes "yes"
#define XMLCACHEDSTRING_no "no"
#define XMLCACHEDSTRING_type "type"
#define XMLCACHEDSTRING_name "name"
#define XMLCACHEDSTRING_id "id"
#define XMLCACHEDSTRING_multiplicator "multiplicator"
#define XMLCACHEDSTRING_number "number"
#define XMLCACHEDSTRING_to "to"
#define XMLCACHEDSTRING_dotcoma ";"
#define XMLCACHEDSTRING_list "list"
#define XMLCACHEDSTRING_monster "monster"
#define XMLCACHEDSTRING_monsters "monsters"
#define XMLCACHEDSTRING_dotxml ".xml"
#define XMLCACHEDSTRING_skills "skills"
#define XMLCACHEDSTRING_buffs "buffs"
#define XMLCACHEDSTRING_egg_step "egg_step"
#define XMLCACHEDSTRING_xp_for_max_level "xp_for_max_level"
#define XMLCACHEDSTRING_xp_max "xp_max"
#define XMLCACHEDSTRING_hp "hp"
#define XMLCACHEDSTRING_attack "attack"
#define XMLCACHEDSTRING_defense "defense"
#define XMLCACHEDSTRING_special_attack "special_attack"
#define XMLCACHEDSTRING_special_defense "special_defense"
#define XMLCACHEDSTRING_speed "speed"
#define XMLCACHEDSTRING_give_sp "give_sp"
#define XMLCACHEDSTRING_give_xp "give_xp"
#define XMLCACHEDSTRING_catch_rate "catch_rate"
#define XMLCACHEDSTRING_type2 "type2"
#define XMLCACHEDSTRING_pow "pow"
#define XMLCACHEDSTRING_ratio_gender "ratio_gender"
#define XMLCACHEDSTRING_percent "%"
#define XMLCACHEDSTRING_attack_list "attack_list"
#define XMLCACHEDSTRING_skill "skill"
#define XMLCACHEDSTRING_skill_level "skill_level"
#define XMLCACHEDSTRING_attack_level "attack_level"
#define XMLCACHEDSTRING_level "level"
#define XMLCACHEDSTRING_byitem "byitem"
#define XMLCACHEDSTRING_evolution "evolution"
#define XMLCACHEDSTRING_evolutions "evolutions"
#define XMLCACHEDSTRING_trade "trade"
#define XMLCACHEDSTRING_evolveTo "evolveTo"
#define XMLCACHEDSTRING_item "item"
#define XMLCACHEDSTRING_fights "fights"
#define XMLCACHEDSTRING_fight "fight"
#define XMLCACHEDSTRING_gain "gain"
#define XMLCACHEDSTRING_cash "cash"
#define XMLCACHEDSTRING_sp "sp"
#define XMLCACHEDSTRING_effect "effect"
#define XMLCACHEDSTRING_endurance "endurance"
#define XMLCACHEDSTRING_life "life"
#define XMLCACHEDSTRING_applyOn "applyOn"
#define XMLCACHEDSTRING_quantity "quantity"
#define XMLCACHEDSTRING_more "+"
#define XMLCACHEDSTRING_success "success"
#define XMLCACHEDSTRING_buff "buff"
#define XMLCACHEDSTRING_capture_bonus "capture_bonus"
#define XMLCACHEDSTRING_duration "duration"
#define XMLCACHEDSTRING_Always "Always"
#define XMLCACHEDSTRING_NumberOfTurn "NumberOfTurn"
#define XMLCACHEDSTRING_durationNumberOfTurn "durationNumberOfTurn"
#define XMLCACHEDSTRING_ThisFight "ThisFight"
#define XMLCACHEDSTRING_inFight "inFight"
#define XMLCACHEDSTRING_inWalk "inWalk"
#define XMLCACHEDSTRING_steps "steps"
#define XMLCACHEDSTRING_nobody "nobody"
#define XMLCACHEDSTRING_allAlly "allAlly"
#define XMLCACHEDSTRING_allEnemy "allEnemy"
#define XMLCACHEDSTRING_themself "themself"
#define XMLCACHEDSTRING_aloneEnemy "aloneEnemy"
#define XMLCACHEDSTRING_width "width"
#define XMLCACHEDSTRING_height "height"
#define XMLCACHEDSTRING_properties "properties"
#define XMLCACHEDSTRING_property "property"
#define XMLCACHEDSTRING_value "value"
#define XMLCACHEDSTRING_tilewidth "tilewidth"
#define XMLCACHEDSTRING_tileheight "tileheight"
#define XMLCACHEDSTRING_objectgroup "objectgroup"
#define XMLCACHEDSTRING_object "object"
#define XMLCACHEDSTRING_x "x"
#define XMLCACHEDSTRING_y "y"
#define XMLCACHEDSTRING_layer "layer"
#define XMLCACHEDSTRING_data "data"
#define XMLCACHEDSTRING_encoding "encoding"
#define XMLCACHEDSTRING_compression "compression"
#define XMLCACHEDSTRING_base64 "base64"
#define XMLCACHEDSTRING_zlib "zlib"
#define XMLCACHEDSTRING_minLevel "minLevel"
#define XMLCACHEDSTRING_maxLevel "maxLevel"
#define XMLCACHEDSTRING_luck "luck"
#define XMLCACHEDSTRING_condition "condition"
#define XMLCACHEDSTRING_quest "quest"
#define XMLCACHEDSTRING_fightBot "fightBot"
#define XMLCACHEDSTRING_zone "zone"
#define XMLCACHEDSTRING_capture "capture"
#define XMLCACHEDSTRING_fightId "fightId"
#define XMLCACHEDSTRING_shop "shop"
#define XMLCACHEDSTRING_fightid "fightid"
#define XMLCACHEDSTRING_drops "drops"
#define XMLCACHEDSTRING_drop "drop"
#define XMLCACHEDSTRING_quantity_min "quantity_min"
#define XMLCACHEDSTRING_quantity_max "quantity_max"

#endif

#include <string>

#define CACHEDSTRING_yes CachedString::cachedString->yes
#define CACHEDSTRING_no CachedString::cachedString->no
#define CACHEDSTRING_type CachedString::cachedString->type
#define CACHEDSTRING_name CachedString::cachedString->name
#define CACHEDSTRING_id CachedString::cachedString->id
#define CACHEDSTRING_multiplicator CachedString::cachedString->multiplicator
#define CACHEDSTRING_number CachedString::cachedString->number
#define CACHEDSTRING_to CachedString::cachedString->to
#define CACHEDSTRING_dotcoma CachedString::cachedString->dotcoma
#define CACHEDSTRING_list CachedString::cachedString->list
#define CACHEDSTRING_monster CachedString::cachedString->monster
#define CACHEDSTRING_monsters CachedString::cachedString->monsters
#define CACHEDSTRING_dotxml CachedString::cachedString->dotxml
#define CACHEDSTRING_skills CachedString::cachedString->skills
#define CACHEDSTRING_buffs CachedString::cachedString->buffs
#define CACHEDSTRING_egg_step CachedString::cachedString->egg_step
#define CACHEDSTRING_xp_for_max_level CachedString::cachedString->xp_for_max_level
#define CACHEDSTRING_xp_max CachedString::cachedString->xp_max
#define CACHEDSTRING_hp CachedString::cachedString->hp
#define CACHEDSTRING_attack CachedString::cachedString->attack
#define CACHEDSTRING_defense CachedString::cachedString->defense
#define CACHEDSTRING_special_attack CachedString::cachedString->special_attack
#define CACHEDSTRING_special_defense CachedString::cachedString->special_defense
#define CACHEDSTRING_speed CachedString::cachedString->speed
#define CACHEDSTRING_give_sp CachedString::cachedString->give_sp
#define CACHEDSTRING_give_xp CachedString::cachedString->give_xp
#define CACHEDSTRING_catch_rate CachedString::cachedString->catch_rate
#define CACHEDSTRING_type2 CachedString::cachedString->type2
#define CACHEDSTRING_pow CachedString::cachedString->pow
#define CACHEDSTRING_ratio_gender CachedString::cachedString->ratio_gender
#define CACHEDSTRING_percent CachedString::cachedString->percent
#define CACHEDSTRING_attack_list CachedString::cachedString->attack_list
#define CACHEDSTRING_skill CachedString::cachedString->skill
#define CACHEDSTRING_skill_level CachedString::cachedString->skill_level
#define CACHEDSTRING_attack_level CachedString::cachedString->attack_level
#define CACHEDSTRING_level CachedString::cachedString->level
#define CACHEDSTRING_byitem CachedString::cachedString->byitem
#define CACHEDSTRING_evolution CachedString::cachedString->evolution
#define CACHEDSTRING_evolutions CachedString::cachedString->evolutions
#define CACHEDSTRING_trade CachedString::cachedString->trade
#define CACHEDSTRING_evolveTo CachedString::cachedString->evolveTo
#define CACHEDSTRING_item CachedString::cachedString->item
#define CACHEDSTRING_fights CachedString::cachedString->fights
#define CACHEDSTRING_fight CachedString::cachedString->fight
#define CACHEDSTRING_gain CachedString::cachedString->gain
#define CACHEDSTRING_cash CachedString::cachedString->cash
#define CACHEDSTRING_sp CachedString::cachedString->sp
#define CACHEDSTRING_effect CachedString::cachedString->effect
#define CACHEDSTRING_endurance CachedString::cachedString->endurance
#define CACHEDSTRING_life CachedString::cachedString->life
#define CACHEDSTRING_applyOn CachedString::cachedString->applyOn
#define CACHEDSTRING_quantity CachedString::cachedString->quantity
#define CACHEDSTRING_more CachedString::cachedString->more
#define CACHEDSTRING_success CachedString::cachedString->success
#define CACHEDSTRING_buff CachedString::cachedString->buff
#define CACHEDSTRING_capture_bonus CachedString::cachedString->capture_bonus
#define CACHEDSTRING_duration CachedString::cachedString->duration
#define CACHEDSTRING_Always CachedString::cachedString->Always
#define CACHEDSTRING_NumberOfTurn CachedString::cachedString->NumberOfTurn
#define CACHEDSTRING_durationNumberOfTurn CachedString::cachedString->durationNumberOfTurn
#define CACHEDSTRING_ThisFight CachedString::cachedString->ThisFight
#define CACHEDSTRING_inFight CachedString::cachedString->inFight
#define CACHEDSTRING_inWalk CachedString::cachedString->inWalk
#define CACHEDSTRING_steps CachedString::cachedString->steps
#define CACHEDSTRING_nobody CachedString::cachedString->nobody
#define CACHEDSTRING_allAlly CachedString::cachedString->allAlly
#define CACHEDSTRING_allEnemy CachedString::cachedString->allEnemy
#define CACHEDSTRING_themself CachedString::cachedString->themself
#define CACHEDSTRING_aloneEnemy CachedString::cachedString->aloneEnemy
#define CACHEDSTRING_width CachedString::cachedString->width
#define CACHEDSTRING_height CachedString::cachedString->height
#define CACHEDSTRING_properties CachedString::cachedString->properties
#define CACHEDSTRING_property CachedString::cachedString->property
#define CACHEDSTRING_value CachedString::cachedString->value
#define CACHEDSTRING_tilewidth CachedString::cachedString->tilewidth
#define CACHEDSTRING_tileheight CachedString::cachedString->tileheight
#define CACHEDSTRING_objectgroup CachedString::cachedString->objectgroup
#define CACHEDSTRING_object CachedString::cachedString->object
#define CACHEDSTRING_x CachedString::cachedString->x
#define CACHEDSTRING_y CachedString::cachedString->y

#define CACHEDSTRING_borderleft CachedString::cachedString->borderleft
#define CACHEDSTRING_borderright CachedString::cachedString->borderright
#define CACHEDSTRING_bordertop CachedString::cachedString->bordertop
#define CACHEDSTRING_borderbottom CachedString::cachedString->borderbottom
#define CACHEDSTRING_map CachedString::cachedString->map
#define CACHEDSTRING_condition_file CachedString::cachedString->condition_file
#define CACHEDSTRING_condition_id CachedString::cachedString->condition_id
#define CACHEDSTRING_bot CachedString::cachedString->bot
#define CACHEDSTRING_skin CachedString::cachedString->skin
#define CACHEDSTRING_lookAt CachedString::cachedString->lookAt
#define CACHEDSTRING_move CachedString::cachedString->move
#define CACHEDSTRING_file CachedString::cachedString->file
#define CACHEDSTRING_infinite CachedString::cachedString->infinite
#define CACHEDSTRING_true CachedString::cachedString->text_true
#define CACHEDSTRING_visible CachedString::cachedString->visible
#define CACHEDSTRING_false CachedString::cachedString->text_false
#define CACHEDSTRING_layer CachedString::cachedString->layer
#define CACHEDSTRING_data CachedString::cachedString->data
#define CACHEDSTRING_encoding CachedString::cachedString->encoding
#define CACHEDSTRING_compression CachedString::cachedString->compression
#define CACHEDSTRING_base64 CachedString::cachedString->base64
#define CACHEDSTRING_zlib CachedString::cachedString->zlib
#define CACHEDSTRING_minLevel CachedString::cachedString->minLevel
#define CACHEDSTRING_maxLevel CachedString::cachedString->maxLevel
#define CACHEDSTRING_luck CachedString::cachedString->luck
#define CACHEDSTRING_condition CachedString::cachedString->condition
#define CACHEDSTRING_quest CachedString::cachedString->quest
#define CACHEDSTRING_fightBot CachedString::cachedString->fightBot
#define CACHEDSTRING_cave CachedString::cachedString->cave
#define CACHEDSTRING_clan CachedString::cachedString->clan
#define CACHEDSTRING_slash CachedString::cachedString->slash
#define CACHEDSTRING_zone CachedString::cachedString->zone
#define CACHEDSTRING_capture CachedString::cachedString->capture
#define CACHEDSTRING_fightId CachedString::cachedString->fightId
#define CACHEDSTRING_shop CachedString::cachedString->shop
#define CACHEDSTRING_fightid CachedString::cachedString->fightid
#define CACHEDSTRING_dottmx CachedString::cachedString->dottmx
#define CACHEDSTRING_antislash CachedString::cachedString->antislash
#define CACHEDSTRING_learn CachedString::cachedString->learn
#define CACHEDSTRING_heal CachedString::cachedString->heal
#define CACHEDSTRING_market CachedString::cachedString->market
#define CACHEDSTRING_zonecapture CachedString::cachedString->zonecapture
#define CACHEDSTRING_left CachedString::cachedString->left
#define CACHEDSTRING_right CachedString::cachedString->right
#define CACHEDSTRING_top CachedString::cachedString->top
#define CACHEDSTRING_bottom CachedString::cachedString->bottom
#define CACHEDSTRING_fightRange CachedString::cachedString->fightRange
#define CACHEDSTRING_drops CachedString::cachedString->drops
#define CACHEDSTRING_drop CachedString::cachedString->drop
#define CACHEDSTRING_quantity_min CachedString::cachedString->quantity_min
#define CACHEDSTRING_quantity_max CachedString::cachedString->quantity_max

class CachedString
{
public:
    CachedString();
    static CachedString *cachedString;
public:
    std::string yes;
    std::string no;
    std::string type;
    std::string name;
    std::string id;
    std::string multiplicator;
    std::string number;
    std::string to;
    std::string dotcoma;
    std::string list;
    std::string monster;
    std::string monsters;
    std::string dotxml;
    std::string skills;
    std::string buffs;
    std::string egg_step;
    std::string xp_for_max_level;
    std::string xp_max;
    std::string hp;
    std::string attack;
    std::string defense;
    std::string special_attack;
    std::string special_defense;
    std::string speed;
    std::string give_sp;
    std::string give_xp;
    std::string catch_rate;
    std::string type2;
    std::string pow;
    std::string ratio_gender;
    std::string percent;
    std::string attack_list;
    std::string skill;
    std::string skill_level;
    std::string attack_level;
    std::string level;
    std::string byitem;
    std::string evolution;
    std::string evolutions;
    std::string trade;
    std::string evolveTo;
    std::string item;
    std::string fights;
    std::string fight;
    std::string gain;
    std::string cash;
    std::string sp;
    std::string effect;
    std::string endurance;
    std::string life;
    std::string applyOn;
    std::string quantity;
    std::string more;
    std::string success;
    std::string buff;
    std::string capture_bonus;
    std::string duration;
    std::string Always;
    std::string NumberOfTurn;
    std::string durationNumberOfTurn;
    std::string ThisFight;
    std::string inFight;
    std::string inWalk;
    std::string steps;
    std::string nobody;
    std::string allAlly;
    std::string allEnemy;
    std::string themself;
    std::string aloneEnemy;
    std::string width;
    std::string height;
    std::string properties;
    std::string property;
    std::string value;
    std::string tilewidth;
    std::string tileheight;
    std::string objectgroup;
    std::string object;
    std::string x;
    std::string y;

    std::string borderleft;
    std::string borderright;
    std::string bordertop;
    std::string borderbottom;
    std::string map;
    std::string condition_file;
    std::string condition_id;
    std::string bot;
    std::string skin;
    std::string lookAt;
    std::string move;
    std::string file;
    std::string infinite;
    std::string text_true;
    std::string visible;
    std::string text_false;
    std::string layer;
    std::string data;
    std::string encoding;
    std::string compression;
    std::string base64;
    std::string zlib;
    std::string zstd;
    std::string minLevel;
    std::string maxLevel;
    std::string luck;
    std::string condition;
    std::string quest;
    std::string fightBot;
    std::string cave;
    std::string clan;
    std::string slash;
    std::string zone;
    std::string capture;
    std::string fightId;
    std::string shop;
    std::string fightid;
    std::string dottmx;
    std::string antislash;
    std::string learn;
    std::string heal;
    std::string market;
    std::string zonecapture;
    std::string left;
    std::string right;
    std::string top;
    std::string bottom;
    std::string fightRange;
    std::string drops;
    std::string drop;
    std::string quantity_min;
    std::string quantity_max;
};

#endif // CACHEDSTRING_H

#else

#if defined(CATCHCHALLENGER_NATIVEMODESTDSTRING)
#define CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(a) std::string(a)
#else
#define CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(a) a
#endif

#define XMLCACHEDSTRING_yes CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_yes)
#define XMLCACHEDSTRING_no CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_no)
#define XMLCACHEDSTRING_type CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_type)
#define XMLCACHEDSTRING_name CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_name)
#define XMLCACHEDSTRING_id CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_id)
#define XMLCACHEDSTRING_multiplicator CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_multiplicator)
#define XMLCACHEDSTRING_number CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_number)
#define XMLCACHEDSTRING_to CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_to)
#define XMLCACHEDSTRING_dotcoma CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_dotcoma)
#define XMLCACHEDSTRING_list CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_list)
#define XMLCACHEDSTRING_monster CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_monster)
#define XMLCACHEDSTRING_monsters CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_monsters)
#define XMLCACHEDSTRING_dotxml CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_dotxml)
#define XMLCACHEDSTRING_skills CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_skills)
#define XMLCACHEDSTRING_buffs CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_buffs)
#define XMLCACHEDSTRING_egg_step CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_egg_step)
#define XMLCACHEDSTRING_xp_for_max_level CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_xp_for_max_level)
#define XMLCACHEDSTRING_xp_max CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_xp_max)
#define XMLCACHEDSTRING_hp CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_hp)
#define XMLCACHEDSTRING_attack CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_attack)
#define XMLCACHEDSTRING_defense CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_defense)
#define XMLCACHEDSTRING_special_attack CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_special_attack)
#define XMLCACHEDSTRING_special_defense CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_special_defense)
#define XMLCACHEDSTRING_speed CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_speed)
#define XMLCACHEDSTRING_give_sp CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_give_sp)
#define XMLCACHEDSTRING_give_xp CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_give_xp)
#define XMLCACHEDSTRING_catch_rate CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_catch_rate)
#define XMLCACHEDSTRING_type2 CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_type2)
#define XMLCACHEDSTRING_pow CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_pow)
#define XMLCACHEDSTRING_ratio_gender CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_ratio_gender)
#define XMLCACHEDSTRING_percent CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_percent)
#define XMLCACHEDSTRING_attack_list CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_attack_list)
#define XMLCACHEDSTRING_skill CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_skill)
#define XMLCACHEDSTRING_skill_level CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_skill_level)
#define XMLCACHEDSTRING_attack_level CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_attack_level)
#define XMLCACHEDSTRING_level CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_level)
#define XMLCACHEDSTRING_byitem CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_byitem)
#define XMLCACHEDSTRING_evolution CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_evolution)
#define XMLCACHEDSTRING_evolutions CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_evolutions)
#define XMLCACHEDSTRING_trade CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_trade)
#define XMLCACHEDSTRING_evolveTo CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_evolveTo)
#define XMLCACHEDSTRING_item CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_item)
#define XMLCACHEDSTRING_fights CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_fights)
#define XMLCACHEDSTRING_fight CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_fight)
#define XMLCACHEDSTRING_gain CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_gain)
#define XMLCACHEDSTRING_cash CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_cash)
#define XMLCACHEDSTRING_sp CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_sp)
#define XMLCACHEDSTRING_effect CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_effect)
#define XMLCACHEDSTRING_endurance CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_endurance)
#define XMLCACHEDSTRING_life CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_life)
#define XMLCACHEDSTRING_applyOn CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_applyOn)
#define XMLCACHEDSTRING_quantity CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_quantity)
#define XMLCACHEDSTRING_more CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_more)
#define XMLCACHEDSTRING_success CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_success)
#define XMLCACHEDSTRING_buff CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_buff)
#define XMLCACHEDSTRING_capture_bonus CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_capture_bonus)
#define XMLCACHEDSTRING_duration CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_duration)
#define XMLCACHEDSTRING_Always CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_Always)
#define XMLCACHEDSTRING_NumberOfTurn CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_NumberOfTurn)
#define XMLCACHEDSTRING_durationNumberOfTurn CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_durationNumberOfTurn)
#define XMLCACHEDSTRING_ThisFight CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_ThisFight)
#define XMLCACHEDSTRING_inFight CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_inFight)
#define XMLCACHEDSTRING_inWalk CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_inWalk)
#define XMLCACHEDSTRING_steps CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_steps)
#define XMLCACHEDSTRING_nobody CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_nobody)
#define XMLCACHEDSTRING_allAlly CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_allAlly)
#define XMLCACHEDSTRING_allEnemy CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_allEnemy)
#define XMLCACHEDSTRING_themself CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_themself)
#define XMLCACHEDSTRING_aloneEnemy CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_aloneEnemy)
#define XMLCACHEDSTRING_width CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_width)
#define XMLCACHEDSTRING_height CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_height)
#define XMLCACHEDSTRING_properties CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_properties)
#define XMLCACHEDSTRING_property CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_property)
#define XMLCACHEDSTRING_value CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_value)
#define XMLCACHEDSTRING_tilewidth CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_tilewidth)
#define XMLCACHEDSTRING_tileheight CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_tileheight)
#define XMLCACHEDSTRING_objectgroup CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_objectgroup)
#define XMLCACHEDSTRING_object CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_object)
#define XMLCACHEDSTRING_x CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_x)
#define XMLCACHEDSTRING_y CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_y)
#define XMLCACHEDSTRING_layer CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_layer)
#define XMLCACHEDSTRING_data CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_data)
#define XMLCACHEDSTRING_encoding CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_encoding)
#define XMLCACHEDSTRING_compression CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_compression)
#define XMLCACHEDSTRING_base64 CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_base64)
#define XMLCACHEDSTRING_zlib CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_zlib)
#define XMLCACHEDSTRING_zstd CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_zstd)
#define XMLCACHEDSTRING_minLevel CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_minLevel)
#define XMLCACHEDSTRING_maxLevel CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_maxLevel)
#define XMLCACHEDSTRING_luck CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_luck)
#define XMLCACHEDSTRING_condition CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_condition)
#define XMLCACHEDSTRING_quest CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_quest)
#define XMLCACHEDSTRING_fightBot CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_fightBot)
#define XMLCACHEDSTRING_zone CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_zone)
#define XMLCACHEDSTRING_capture CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_capture)
#define XMLCACHEDSTRING_fightId CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_fightId)
#define XMLCACHEDSTRING_shop CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_shop)
#define XMLCACHEDSTRING_fightid CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_fightid)
#define XMLCACHEDSTRING_drops CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_drops)
#define XMLCACHEDSTRING_drop CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_drop)
#define XMLCACHEDSTRING_quantity_min CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_quantity_min)
#define XMLCACHEDSTRING_quantity_max CATCHCHALLENGER_XMLDOCUMENTTONATIVETYPE(CACHEDSTRING_quantity_max)

#define CACHEDSTRING_yes "yes"
#define CACHEDSTRING_no "no"
#define CACHEDSTRING_type "type"
#define CACHEDSTRING_name "name"
#define CACHEDSTRING_id "id"
#define CACHEDSTRING_multiplicator "multiplicator"
#define CACHEDSTRING_number "number"
#define CACHEDSTRING_to "to"
#define CACHEDSTRING_dotcoma ";"
#define CACHEDSTRING_list "list"
#define CACHEDSTRING_monster "monster"
#define CACHEDSTRING_monsters "monsters"
#define CACHEDSTRING_dotxml ".xml"
#define CACHEDSTRING_skills "skills"
#define CACHEDSTRING_buffs "buffs"
#define CACHEDSTRING_egg_step "egg_step"
#define CACHEDSTRING_xp_for_max_level "xp_for_max_level"
#define CACHEDSTRING_xp_max "xp_max"
#define CACHEDSTRING_hp "hp"
#define CACHEDSTRING_attack "attack"
#define CACHEDSTRING_defense "defense"
#define CACHEDSTRING_special_attack "special_attack"
#define CACHEDSTRING_special_defense "special_defense"
#define CACHEDSTRING_speed "speed"
#define CACHEDSTRING_give_sp "give_sp"
#define CACHEDSTRING_give_xp "give_xp"
#define CACHEDSTRING_catch_rate "catch_rate"
#define CACHEDSTRING_type2 "type2"
#define CACHEDSTRING_pow "pow"
#define CACHEDSTRING_ratio_gender "ratio_gender"
#define CACHEDSTRING_percent "%"
#define CACHEDSTRING_attack_list "attack_list"
#define CACHEDSTRING_skill "skill"
#define CACHEDSTRING_skill_level "skill_level"
#define CACHEDSTRING_attack_level "attack_level"
#define CACHEDSTRING_level "level"
#define CACHEDSTRING_byitem "byitem"
#define CACHEDSTRING_evolution "evolution"
#define CACHEDSTRING_evolutions "evolutions"
#define CACHEDSTRING_trade "trade"
#define CACHEDSTRING_evolveTo "evolveTo"
#define CACHEDSTRING_item "item"
#define CACHEDSTRING_fights "fights"
#define CACHEDSTRING_fight "fight"
#define CACHEDSTRING_gain "gain"
#define CACHEDSTRING_cash "cash"
#define CACHEDSTRING_sp "sp"
#define CACHEDSTRING_effect "effect"
#define CACHEDSTRING_endurance "endurance"
#define CACHEDSTRING_life "life"
#define CACHEDSTRING_applyOn "applyOn"
#define CACHEDSTRING_quantity "quantity"
#define CACHEDSTRING_more "+"
#define CACHEDSTRING_success "success"
#define CACHEDSTRING_buff "buff"
#define CACHEDSTRING_capture_bonus "capture_bonus"
#define CACHEDSTRING_duration "duration"
#define CACHEDSTRING_Always "Always"
#define CACHEDSTRING_NumberOfTurn "NumberOfTurn"
#define CACHEDSTRING_durationNumberOfTurn "durationNumberOfTurn"
#define CACHEDSTRING_ThisFight "ThisFight"
#define CACHEDSTRING_inFight "inFight"
#define CACHEDSTRING_inWalk "inWalk"
#define CACHEDSTRING_steps "steps"
#define CACHEDSTRING_nobody "nobody"
#define CACHEDSTRING_allAlly "allAlly"
#define CACHEDSTRING_allEnemy "allEnemy"
#define CACHEDSTRING_themself "themself"
#define CACHEDSTRING_aloneEnemy "aloneEnemy"
#define CACHEDSTRING_width "width"
#define CACHEDSTRING_height "height"
#define CACHEDSTRING_properties "properties"
#define CACHEDSTRING_property "property"
#define CACHEDSTRING_value "value"
#define CACHEDSTRING_tilewidth "tilewidth"
#define CACHEDSTRING_tileheight "tileheight"
#define CACHEDSTRING_objectgroup "objectgroup"
#define CACHEDSTRING_object "object"
#define CACHEDSTRING_x "x"
#define CACHEDSTRING_y "y"

#define CACHEDSTRING_borderleft "border-left"
#define CACHEDSTRING_borderright "border-right"
#define CACHEDSTRING_bordertop "border-top"
#define CACHEDSTRING_borderbottom "border-bottom"
#define CACHEDSTRING_map "map"
#define CACHEDSTRING_condition_file "condition-file"
#define CACHEDSTRING_condition_id "condition-id"
#define CACHEDSTRING_bot "bot"
#define CACHEDSTRING_skin "skin"
#define CACHEDSTRING_lookAt "lookAt"
#define CACHEDSTRING_move "move"
#define CACHEDSTRING_file "file"
#define CACHEDSTRING_infinite "infinite"
#define CACHEDSTRING_true "true"
#define CACHEDSTRING_visible "visible"
#define CACHEDSTRING_false "false"
#define CACHEDSTRING_layer "layer"
#define CACHEDSTRING_data "data"
#define CACHEDSTRING_encoding "encoding"
#define CACHEDSTRING_compression "compression"
#define CACHEDSTRING_base64 "base64"
#define CACHEDSTRING_zlib "zlib"
#define CACHEDSTRING_zstd "zstd"
#define CACHEDSTRING_minLevel "minLevel"
#define CACHEDSTRING_maxLevel "maxLevel"
#define CACHEDSTRING_luck "luck"
#define CACHEDSTRING_condition "condition"
#define CACHEDSTRING_quest "quest"
#define CACHEDSTRING_fightBot "fightBot"
#define CACHEDSTRING_cave "cave"
#define CACHEDSTRING_clan "clan"
#define CACHEDSTRING_slash "/"
#define CACHEDSTRING_zone "zone"
#define CACHEDSTRING_capture "capture"
#define CACHEDSTRING_fightId "fightId"
#define CACHEDSTRING_shop "shop"
#define CACHEDSTRING_fightid "fightid"
#define CACHEDSTRING_dottmx ".tmx"
#define CACHEDSTRING_antislash "\\"
#define CACHEDSTRING_learn "learn"
#define CACHEDSTRING_heal "heal"
#define CACHEDSTRING_market "market"
#define CACHEDSTRING_zonecapture "zonecapture"
#define CACHEDSTRING_left "left"
#define CACHEDSTRING_right "right"
#define CACHEDSTRING_top "top"
#define CACHEDSTRING_bottom "bottom"
#define CACHEDSTRING_fightRange "fightRange"
#define CACHEDSTRING_drops "drops"
#define CACHEDSTRING_drop "drop"
#define CACHEDSTRING_quantity_min "quantity_min"
#define CACHEDSTRING_quantity_max "quantity_max"

#endif
