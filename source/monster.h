#pragma once

#include "3dstypes.h"

#define MAX_POINTERS_IN_LIST 10
#define MAX_PARTS_PER_MONSTER 8

#pragma pack(push,1)

//Parts info
typedef struct
{
  u16 unknown;		//0x0
  u32 fixed;		//0x2: F8 99 D0 00
  u8 stagger_count;	//0x6
  u8 break_count;	//0x7
  u16 stagger_hp;	//0x8
  s16 break_hp;		//0xA	
} Part;

//Monster info (incomplete)
typedef struct
{
  u8 identifier1;   //0x0: similar monsters may have same value (rathalos and rathian)
  u8 pad_0x1[0xC];
  u8 location_flag; //0xD: 4C = in current map, 44 = not in current map
  u8 pad_0xE[0x2];
  u8 is_despawned;  //0x10: 7 = despawned, F = visible
  u8 pad_0x11[0x217];
  u8 identifier2;   //0x228: may define some kind of grouping, but not obvious
  u8 pad_0x229[0xE57];
  u16 action;		//0x1080: monster action state (FFFF for small monsters)
  u8 pad_0x1082[0x12E];
  u8 test1;			//0x11B0
  u8 pad_0x11B1[0xB1];
  u8 identifier3;	//0x1262: 00 or 80 for small monsters, 20 for ~drome type, 60 for Great Maccao, 08 for Kirin, otherwise ...
					//		  04 or 44 for regular monsters, 0C or 4C for hyper monsters or special monsters (eg. Alatreon)
					//		  Note: deviant/special-form monsters can have either, depending on the species, so not useful for differentiating those
  u8 pad_0x1263[0x3];
  u8 identifier4;	//0x1266: unique for each monster, except ~drome type, rathian/rathalos, hyper monsters, special-form; can be used to tell regular vs deviant
  u8 pad_0x1267[0x1B1];
  u32 hp;			//0x1418
  u32 max_hp;		//0x141C
  u8 pad_0x1420[0x3E];
  Part parts[8];	//0x145E
  u8 pad_0x14BE[0x543E];
  u16 poison;       //0x68FC
  u16 max_sleep;    //0x68FE: unlike other max values, only increases after abnormal state ends (e.g. awakens)
  u16 sleep;        //0x6900
  u8 pad_0x6902[0x6];
  u16 max_poison;   //0x6908
  u8 pad_0x690A[0x2];
  u32 poison_timer; //0x690C
  u32 poison_deactivate_timer; //0x6910: starts after the poison kicks in
  u16 max_paralysis;//0x6914
  u16 paralysis;    //0x6916
  u32 paralysis_timer; //0x6918
  u8 pad_0x691C[0x4E];
  u8 is_asleep;     //0x696A: 00 normal 01 asleep or dead
  u8 pad_0x696B[0x6B];
  u16 dizzy;        //0x69D6
  u8 pad_0x69D8[0x4];
  u32 dizzy_timer;  //0x69DC
  u8 pad_0x69E0[0x2];
  u16 exhaust;      //0x69E2
  u8 pad_0x69E4[0x4];
  u32 exhaust_timer;//0x69E8
  u8 pad_0x69EC[0x2];
  u16 stumble; 		//0x69EE: need testing, increases when jump attacks land, resets to 0 once monster falls
  u8 pad_0x69F0[0xA];
  u16 jump;         //0x69FA
  u16 max_jump;     //0x69FC
  u8 pad_0x69FE[0x6];
  u16 ride_counter; //0x6A04: increase by 1 when jump attack reaches threshold, and increases by 256 when ride finishes regardless of success
  u8 blast_counter; //0x6A06: increase by 1 everytime a blast occurs
  u8 pad_0x6A07[0x1];
  u16 max_blast;    //0x6A08
  u16 blast;        //0x6A0A
  u8 pad_0x6A0C[0x4];
  u32 blast_timer;  //0x6A10
  u8 pad_0x6A14[0x50];
  u8 status;        //0x6A64: indicates active abnormal stats: sleep=1 poison=2 paralysis=4 dizzy=16
  } Monster;
/* Notes:
naming convention:
	static: always or almost always fixed for a given monster
	fixed: always fixed to some value
	unused: always 0
	vary: changes between quests, but stays constant within the quest
	dynamic: changes within the quest
	pair: has some kind of relationship with another field
  0x1:    2bytes static. Different for each big monster(except special, and changes when not first monster), but for small monsters is B1 4F or B2 4F
  0x3:    1byte fixed. Fixed at 1
  0x4:    8bytes unused.
  0xC:    1byte fixed. Fixed at 22
  0xE:    1byte fixed. Fixed at 1
  0xF:    1byte vary. Mosly 00 for large but can be different (Lv10 Hellblade),varies for small
  0x11:   1byte fixed. Fixed at 8
  0x12:   2bytes unused.
  0x14:   8bytes 2 pointers. Can be null; always null at base camp, both always different
  0x1C:   3bytes dynamic.
  0x1F:   1byte dynamic. 40 for big monsters (sometimes not true), dynamic for small
  0xF0:   1byte pair. Same as 0x230
  0xFC:   1byte dynamic. Small numbers
  0x10C:  4bytes pair. Same as 0x1BC; random value for big, last 3 are FF for small; for small first byte can be dynamic
  0x110:  4bytes vary. Random value for big, FFFFFFFF or 0 for small
  0x118:  1byte dynamic. Starts at FF but changes to BF when low health. Values can be different for different monsters. Same value as 0x1C8 except during initial loading
  0x166:  1byte dynamic. Starts at 00 during initial loading but changes to 01 after quest begins. Spawned monsters don't have this behavior and start at 1
  0x1BC:  4bytes pair. Same as 0x10C
  0x1C8:  1byte dynamic. Starts as 00 during initial loading, changes to same value as 0x118 after quest begins, and changes values with it as well
  0x230:  1byte pair. Same as 0xF0
  0x242:  1byte dynamic.
  0x256:  1byte dynamic.
  0x263:  1byte dynamic. Changes slowly. Some possibilities: 40, 3F
  0x28B:  1byte dynamic.
  0x372:  1byte pair. Sometimes same as 0xC0A, generally small numbers 04~12, not unique for monsters
  0xBCA:  1byte vary. Some possibilities: 0A, 09, 0C, 10, 0F
  0xBCC:  1byte vary. Single digit numbers, seems to be 1 for small but can vary for large
  0xBD0:  1byte vary.
  0xC0A:  1byte pair. Sometimes same as 0x372
  0xC0C:  1byte vary. Almost static
  0xC14:  1byte vary. Almost static
  0xC18:  1byte vary. Almost static
  0xC1C:  1byte vary. Mostly 00 for small monsters, not unique for large monsters
  0xC84:  1byte pair. Same as 0xC8C and 0xF08; varies
  0xC8C:  1byte pair. Same as 0xC84 and 0xF08
  0xCF3:  1byte vary.
  0xD05:  1byte dynamic. Small numbers
  0xD08:  1byte dynamic. Small numbers
  0xD7F:  1byte dynamic. Changes slowly. Some possibilities: 00, 41, 40
  0xEB7:  1byte dynamic. Mostly 31, but can be 00
  0xEF4:  1byte vary. Not unique for monsters
  0xF08:  1byte pair. Same as 0xC84 and 0xC8C
  0xF14:  1byte vary. Not unique for monsters
  0xF19:  1byte vary. Mostly A8, but can be A7, 00, ...
  0xFE8:  1byte dynamic. Small numbers
  *0xFEC: 1 byte vary. Seems to be able to tell velocidrome apart but has multiple values each (depending on quest level?)
  0x1037: 1byte dynamic. Changes very fast. Most commonly 40
  0x103B: 1byte dynamic. Changes slowly. Some possibilities: 40, 3F
  0x1075: 1byte dynamic.
  0x10A0: 1byte vary. Almost static
  0x10A4: 1byte vary.
  0x10A8: 1byte vary.
  0x10F3: 1byte dynamic.
  0x1123: 1byte dynamic. 00 for most monsters, but for rathalos/rathian family can be 00,40,41,42 changing somewhat periodically
  *0x1170: 1byte static. Gold Rajan identifier?
  0x1259: 1byte dynamic. Usually 01
  0x125F: 1byte dynamic. Usually 02
  *0x1260: 1byte static. Gold Rajan identifier?
  0x12C3: 1byte dynamic. Starts at 00, turns to 30 and stays there shortly after quest begins
  0x13B2: 2byte dynamic. Starts at 80 BF, then starts to change quickly once discovered player
  0x13B6: 2byte dynamic. Starts at 80 BF, then starts to change quickly once discovered player
  0x1410: 1byte dynamic. Mostly FF, but can change to something else quickly
  0x1430: 4bytes vary. Some possibilities: 3F59999A, 3F400000, 3F4CCCCD; fixed within quest; can vary between same monster
  *0x1438: 1byte static. Gold Rajan identifier?
  *0x1439: 1byte static. Gold Rajan identifier?
  *0x143A: 1byte static. Gold Rajan identifier?
  0x1445: 1byte vary. Thought was bit pattern that could tell variant monsters from normal (gold rathian vs rathian at bit position 0x40) but turns out it doesn't always work
  0x1447: 1byte vary. 00, 01, or 03 (maybe quest level?)
  0x6A07: 1byte vary.
*/

typedef struct
{
  u8 fixed;         //0x0: fixed at 1
  u8 unused[0xF];   //0x1
  Monster* m[MAX_POINTERS_IN_LIST];   //0x10
  u8 count;         //0x38: number of pointers
} MonsterPointerList;

#pragma pack(pop)

typedef struct
{
  u16 max_stagger_hp;
  s16 max_break_hp;
} PartCache;

typedef struct
{
  Monster* m;       //set to 0 to deactivate
  PartCache p[8];
  u8 display_count; //for displaying only parts that are cuttable
  u16 break_hp_sum; //only sum displayable parts
} MonsterCache;

//static vars
static MonsterCache m_cache[2]; //assume only 2 big monsters are active at a time

inline u8 isSmallMonster(Monster* m)
{
  return m->identifier3 == 0 || m->identifier3 == 0x80;
}

u8 getMonsterCount(MonsterPointerList* list, u8 show_small_monsters);
MonsterCache* getCachedMonsterByIndex(u8 index);
MonsterCache* getCachedMonsterByPointer(Monster* m);
void updateMonsterCache(MonsterPointerList* list);