#pragma pack(push,1)

//Parts info
typedef struct
{
  u16 unknown1;		//0x0
  u32 fixed;		//0x2: F8 99 D0 00
  u8 stagger_count;	//0x6
  u8 break_count;	//0x7
  u16 stagger_hp;	//0x8
  s16 break_hp;		//0xA	
} Part;

//Monster info (incomplete)
typedef struct
{
  u8 static1;       //0x0: potential monster id for big (but special has same byte, not always unique), but random for small
  u16 static2;      //0x1: different for each big monster(except special, and changes when not first monster), but for small monsters is B1 4F or B2 4F
  u8 fixed1;        //0x3: fixed at 1
  u32 unused1[2];   //0x4: unused
  u8 fixed2;        //0xC: fixed at 22
  u8 location_flag; //0xD: 4C = in current map, 44 = not in current map
  u8 fixed3;        //0xE: fixed at 1
  u8 vary1;         //0xF: mosly 00 for large but can be different (Lv10 Hellblade),varies for small
  u8 is_despawned;  //0x10: 7 = despawned, F = visible
  u8 fixed4;		//0x11: fixed at 8
  u16 unused2;		//0x12: unused
  u32 pointer1[2];  //0x14: can be null; always null at base camp, both always different
  u8 dynamic1[3];   //0x1C: changes all the time
  u8 unknown1;      //0x1F: 40 for big monsters (sometimes not true), dynamic for small
  u8 ignore1[0xEC];
  u32 pair1;        //0x10C: same as pair2; random value for big, last 3 are FF for small; for small first byte can be dynamic
  u32 unknown2;     //0x110: random value for big, FFFFFFFF or 0 for small
  u8 ignore2[0xA8];
  u32 pair2;        //0x1BC: same as pair1
  u8 ignore3[0x68];
  u8 static3;       //0x228: potential monster id for big (but special has same byte)
  u8 ignore4[0x9A3];
  u8 unknown3;      //0xBCC: single digit numbers, seems to be 1 for small but can vary for large
  u8 ignore5[0x695];
  u8 test1;			//0x1262: possible identifier for lemon
  u8 ignore6[0x1B5];
  u32 hp;			//0x1418
  u32 max_hp;		//0x141C
  u8 ignore7[0x3E];
  Part parts[8];	//0x145E
} Monster;

//Monster Pointer List
typedef struct
{
  u8 fixed;         //0x0: fixed at 1
  u8 unused[0xF];   //0x1
  Monster* m[10];   //0x10
  u8 count;         //0x38: number of pointers
} MonsterPointerList;

#pragma pack(pop)